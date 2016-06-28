#include <hoomd/hoomd_config.h>
#include <hoomd/HOOMDMath.h>
#include <hoomd/Index1D.h>
#include <hoomd/BoxDim.h>

#include <cufft.h>

void gpu_bin_particles(const unsigned int N,
                       const Scalar4 *d_postype,
                       Scalar4 *d_particle_bins,
                       unsigned int *d_n_cell,
                       unsigned int *d_overflow,
                       const Index2D& bin_idx,
                       const uint3 mesh_dim,
                       const uint3 n_ghost_bins,
                       const Scalar *d_mode,
                       const BoxDim& box);

void gpu_assign_binned_particles_to_mesh(const uint3 mesh_dim,
                                         const uint3 n_ghost_bins,
                                         const uint3 grid_dim,
                                         const Scalar4 *d_particle_bins,
                                         Scalar *d_mesh_scratch,
                                         const Index2D& bin_idx,
                                         const Index2D& scratch_idx,
                                         const unsigned int *d_n_cell,
                                         cufftComplex *d_mesh);

void gpu_compute_mesh_virial(const unsigned int n_wave_vectors,
                             cufftComplex *d_fourier_mesh,
                             cufftComplex *d_fourier_mesh_G,
                             Scalar *d_virial_mesh,
                             const Scalar3 *d_k,
                             const Scalar qstarsq,
                             const bool exclude_dc);

void gpu_update_meshes(const unsigned int n_wave_vectors,
                         cufftComplex *d_fourier_mesh,
                         cufftComplex *d_fourier_mesh_G,
                         const Scalar *d_inf_f,
                         const Scalar3 *d_k,
                         const unsigned int N_global);

void gpu_compute_forces(const unsigned int N,
                        const Scalar4 *d_postype,
                        Scalar4 *d_force,
                        const Scalar bias,
                        const cufftComplex *d_inv_fourier_mesh,
                        const uint3 grid_dim,
                        const uint3 n_ghost_cells,
                        const Scalar *d_mode,
                        const BoxDim& box,
                        const BoxDim& global_box,
                        const unsigned int n_global);

void gpu_compute_cv(unsigned int n_wave_vectors,
                   Scalar *d_sum_partial,
                   Scalar *d_sum,
                   const cufftComplex *d_fourier_mesh,
                   const cufftComplex *d_fourier_mesh_G,
                   const unsigned int block_size,
                   const uint3 mesh_dim,
                   const bool exclude_dc);

void gpu_compute_virial(unsigned int n_wave_vectors,
                   Scalar *d_sum_virial_partial,
                   Scalar *d_sum_virial,
                   const Scalar *d_mesh_virial,
                   const unsigned int block_size);

void gpu_compute_influence_function(const uint3 mesh_dim,
                                    const uint3 global_dim,
                                    Scalar *d_inf_f,
                                    Scalar3 *d_k,
                                    const BoxDim& global_box,
                                    const Scalar qstarsq,
                                    const int3 *d_zero_modes,
                                    const unsigned int n_zero_modes,
                                    const bool local_fft,
                                    const uint3 pidx,
                                    const uint3 pdim);

void gpu_compute_q_max(unsigned int n_wave_vectors,
                   Scalar4 *d_max_partial,
                   Scalar4 *d_q_max,
                   const Scalar3 *d_k,
                   const cufftComplex *d_fourier_mesh,
                   const unsigned int block_size);
