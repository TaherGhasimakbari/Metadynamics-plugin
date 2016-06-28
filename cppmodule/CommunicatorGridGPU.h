#ifndef __COMMUNICATOR_GRID_GPU_H__
#define __COMMUNICATOR_GRID_GPU_H__

#include <hoomd/hoomd.h>
#ifdef ENABLE_CUDA
#include "CommunicatorGrid.h"

#ifdef ENABLE_MPI
/*! Class to communicate the boundary layer of a regular grid (GPU version)
 */
template<typename T>
class CommunicatorGridGPU : public CommunicatorGrid<T>
    {
    public:
        //! Constructor
        CommunicatorGridGPU(boost::shared_ptr<SystemDefinition> sysdef, uint3 dim,
            uint3 embed, uint3 offset, bool add_outer_layer_to_inner);

        //! Communicate grid
        virtual void communicate(const GPUArray<T>& grid);

    protected:
        unsigned int m_n_unique_recv_cells;       //!< Number of unique receiving cells

        //! Initialize grid communication
        virtual void initGridCommGPU();

    private:
        GPUArray<unsigned int> m_cell_recv;       //!< Array of per-cell receive elements (multiple possible)
        GPUArray<unsigned int> m_cell_recv_begin; //!< Begin of recv indices per cell
        GPUArray<unsigned int> m_cell_recv_end;   //!< End of recv indices per cell
    };

#endif // ENABLE_MPI
#endif // __COMMUNICATOR_GRID_GPU_H__
#endif
