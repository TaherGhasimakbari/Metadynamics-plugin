#include "CommunicatorGridGPU.h"

#ifdef ENABLE_CUDA
#include "CommunicatorGridGPU.cuh"
/*! \param sysdef The system definition
 *  \param dim Dimensions of 3dim grid
 *  \param embed Embedding dimensions
 *  \param offset Start offset of inner grid in array
 *  \param add_outer_layer_to_inner True if outer ghost layer should be added to inner cells
 */
template<typename T>
CommunicatorGridGPU<T>::CommunicatorGridGPU(boost::shared_ptr<SystemDefinition> sysdef, uint3 dim,
            uint3 embed, uint3 offset, bool add_outer_layer_to_inner)
    : CommunicatorGrid<T>(sysdef, dim, embed, offset, add_outer_layer_to_inner),
      m_n_unique_recv_cells(0)
    {
    initGridCommGPU();
    }

template<typename T>
void CommunicatorGridGPU<T>::initGridCommGPU()
    {
    // if we have multiple equal destination cells, group them by destination cell
    ArrayHandle<unsigned int> h_recv_idx(this->m_recv_idx, access_location::host, access_mode::read);
    typedef std::multimap<unsigned int, unsigned int> map_t;
    typedef std::set<unsigned int> set_t;

    map_t map;
    set_t unique_cells;

    // insert keys and values into multimap and map (to count unique elements)
    for (unsigned int i = 0; i < this->m_recv_idx.getNumElements(); ++i)
        {
        unique_cells.insert(h_recv_idx.data[i]);
        map.insert(std::make_pair(h_recv_idx.data[i], i));
        }

    m_n_unique_recv_cells = unique_cells.size();

    // allocate arrays
    GPUArray<unsigned int> cell_recv(this->m_recv_idx.getNumElements(), this->m_exec_conf);
    m_cell_recv.swap(cell_recv);

    GPUArray<unsigned int> cell_recv_begin(m_n_unique_recv_cells, this->m_exec_conf);
    m_cell_recv_begin.swap(cell_recv_begin);

    GPUArray<unsigned int> cell_recv_end(m_n_unique_recv_cells, this->m_exec_conf);
    m_cell_recv_end.swap(cell_recv_end);

    // write out sorted values according to cell idx
    ArrayHandle<unsigned int> h_cell_recv(m_cell_recv, access_location::host, access_mode::overwrite);
    unsigned int n = 0;
    for (map_t::iterator it = map.begin(); it != map.end(); ++it)
        h_cell_recv.data[n++] = it->second;

    // locate beginning and end of each cell
    ArrayHandle<unsigned int> h_cell_recv_begin(m_cell_recv_begin, access_location::host, access_mode::overwrite);
    ArrayHandle<unsigned int> h_cell_recv_end(m_cell_recv_end, access_location::host, access_mode::overwrite);

    n = 0;
    for (set_t::iterator it = unique_cells.begin(); it != unique_cells.end(); ++it)
        {
        map_t::iterator lower = map.lower_bound(*it);
        map_t::iterator upper = map.upper_bound(*it);
        h_cell_recv_begin.data[n] = std::distance(map.begin(), lower);
        h_cell_recv_end.data[n] = std::distance(map.begin(), upper);
        assert(h_cell_recv_end.data[n] - h_cell_recv_begin.data[n] >= 1);
        n++;
        }
    }

template<typename T>
void CommunicatorGridGPU<T>::communicate(const GPUArray<T>& grid)
    {
    assert(grid.getNumElements() >= this->m_embed.x*this->m_embed.y*this->m_embed.z);

        {
        ArrayHandle<T> d_send_buf(this->m_send_buf, access_location::device, access_mode::overwrite);
        ArrayHandle<unsigned int> d_send_idx(this->m_send_idx, access_location::device, access_mode::read);
        ArrayHandle<T> d_grid(grid, access_location::device, access_mode::read);

        gpu_gridcomm_scatter_send_cells<T>(
            this->m_send_buf.getNumElements(),
            d_send_idx.data,
            d_grid.data,
            d_send_buf.data);
        if (this->m_exec_conf->isCUDAErrorCheckingEnabled()) CHECK_CUDA_ERROR();
        }

        {
        // access send and recv buffers
        #ifdef ENABLE_MPI_CUDA
        ArrayHandle<T> send_buf_handle(this->m_send_buf, access_location::device, access_mode::read);
        ArrayHandle<T> recv_buf_handle(this->m_recv_buf, access_location::device, access_mode::overwrite);
        #else
        ArrayHandle<T> send_buf_handle(this->m_send_buf, access_location::host, access_mode::read);
        ArrayHandle<T> recv_buf_handle(this->m_recv_buf, access_location::host, access_mode::overwrite);
        #endif

        typedef std::map<unsigned int, unsigned int>::iterator it_t;
        std::vector<MPI_Request> reqs(2*this->m_neighbors.size());

        unsigned int n = 0;
        for (std::set<unsigned int>::iterator it = this->m_neighbors.begin(); it != this->m_neighbors.end(); it++)
            {
            it_t b = this->m_begin.find(*it);
            assert(b != this->m_begin.end());
            it_t e = this->m_end.find(*it);
            assert(e != this->m_end.end());

            unsigned int offs = b->second;
            unsigned int n_elem = e->second - b->second;

            MPI_Isend(&send_buf_handle.data[offs], n_elem*sizeof(T), MPI_BYTE, *it, 0,
                this->m_exec_conf->getMPICommunicator(), &reqs[n++]);
            MPI_Irecv(&recv_buf_handle.data[offs], n_elem*sizeof(T), MPI_BYTE, *it, 0,
                this->m_exec_conf->getMPICommunicator(), &reqs[n++]);
            }

        std::vector<MPI_Status> stat(reqs.size());
        MPI_Waitall(reqs.size(), &reqs.front(), &stat.front());
        }

        {
        ArrayHandle<T> d_recv_buf(this->m_recv_buf, access_location::device, access_mode::read);
        ArrayHandle<T> d_grid(grid, access_location::device, access_mode::readwrite);

        ArrayHandle<unsigned int> d_cell_recv(m_cell_recv, access_location::device, access_mode::read);
        ArrayHandle<unsigned int> d_cell_recv_begin(m_cell_recv_begin, access_location::device, access_mode::read);
        ArrayHandle<unsigned int> d_cell_recv_end(m_cell_recv_end, access_location::device, access_mode::read);
        ArrayHandle<unsigned int> d_recv_idx(this->m_recv_idx, access_location::device, access_mode::read);

        gpu_gridcomm_scatter_add_recv_cells<T>(
            m_n_unique_recv_cells,
            d_recv_buf.data,
            d_grid.data,
            d_cell_recv.data,
            d_cell_recv_begin.data,
            d_cell_recv_end.data,
            d_recv_idx.data,
            this->m_add_outer);
        if (this->m_exec_conf->isCUDAErrorCheckingEnabled()) CHECK_CUDA_ERROR();
        }
    }

//! Explicit template instantiations
template class CommunicatorGridGPU<Scalar>;
template class CommunicatorGridGPU<unsigned int>;
template class CommunicatorGridGPU<cufftComplex>;
#endif //ENABLE_CUDA
