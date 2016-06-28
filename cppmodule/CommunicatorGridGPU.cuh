#include <hoomd/hoomd_config.h>
#include <hoomd/HOOMDMath.h>

template<typename T>
void gpu_gridcomm_scatter_send_cells(
    unsigned int n_send_cells,
    unsigned int *d_send_idx,
    const T *d_grid,
    T *d_send_buf);

template<typename T>
void gpu_gridcomm_scatter_add_recv_cells(
    unsigned int n_unique_cell_recvs,
    const T *d_recv_buf,
    T *d_grid,
    const unsigned int *d_cell_recv,
    const unsigned int *d_cell_recv_begin,
    const unsigned int *d_cell_recv_end,
    const unsigned int *d_recv_idx,
    bool add_outer);
