#ifndef AMREX_CUDA_DEVICE_H_
#define AMREX_CUDA_DEVICE_H_

#include <cstdlib>
#include <memory>
#include <AMReX.H>
#include <AMReX_Utility.H>

#ifdef AMREX_USE_CUDA
#ifdef AMREX_USE_NVML
#include <nvml.h>
#endif
#endif

#if defined(AMREX_USE_CUDA) && defined(__CUDACC__)

#define CudaErrorCheck() ActualCudaErrorCheck()

inline void ActualCudaErrorCheck() {
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::string errStr("CUDA failure: ");
        errStr.append(cudaGetErrorString(err));
        amrex::Abort(errStr);
    }
}

#define CudaAPICheck(error) ActualCudaAPICheck(error)

inline void ActualCudaAPICheck(cudaError_t err) {
    if (err != cudaSuccess) {
        std::string errStr("CUDA failure: ");
        errStr.append(cudaGetErrorString(err));
        amrex::Abort(errStr);
    }
}

#ifdef AMREX_USE_NVML
#define NvmlAPICheck(error) ActualNvmlAPICheck(error)

inline void ActualNvmlAPICheck(nvmlReturn_t err) {
    if (err != NVML_SUCCESS) {
        std::string errStr("NVML failure: ");
        errStr.append(nvmlErrorString(err));
        amrex::Abort(errStr);
        }
}
#endif

#endif

extern "C" {
#ifdef AMREX_USE_CUDA
    void initialize_cuda(const int* id);
    void finalize_cuda();
    void set_stream_idx(const int idx);
    void get_stream_idx(int* idx);
    void get_cuda_device_id(int* device);
    void gpu_malloc(void** p, const std::size_t* sz);
    void gpu_hostalloc(void** p, const std::size_t* sz);
    void gpu_malloc_managed(void** p, const std::size_t* sz);
    void gpu_free(void* p);
    void gpu_freehost(void* p);
    void gpu_host_device_ptr(void** x, const void* y);
    void gpu_stream_synchronize(const int idx);
    void gpu_synchronize();
    void gpu_start_profiler();
    void gpu_stop_profiler();
    void get_num_SMs(int* num);
    void set_threads_and_blocks(const int* lo, const int* hi,
                                const int* txmin, const int* tymin, const int* tzmin);
    void get_threads_and_blocks(const int* lo, const int* hi,
                                int* bx, int* by, int* bz, int* tx, int* ty, int* tz,
                                const int* txmin, const int* tymin, const int* tzmin);
    __device__ void get_loop_bounds(int* blo, int* bhi, const int* lo, const int* hi);
    void gpu_htod_memcpy_async(void* p_d, const void* p_h, const std::size_t* sz, const int* idx);
    void gpu_dtoh_memcpy_async(void* p_h, const void* p_d, const std::size_t* sz, const int* idx);
    void gpu_htod_memprefetch_async(void* p, const std::size_t* sz, const int* idx);
    void gpu_dtoh_memprefetch_async(void* p, const std::size_t* sz, const int* idx);
    void check_for_gpu_errors();
    void set_is_program_running(int r);
#endif
}

namespace amrex {
namespace Cuda {

class Device
{

public:

    static void initialize_device();

    static void finalize_device();

    static void set_stream_index(const int idx);

    static int  get_stream_index();

    static void prepare_for_launch(const int* lo, const int* hi);

    static void* get_host_pointer(const void* ptr);

    static int deviceId();

    static void check_for_errors();

    static void synchronize();

    static void stream_synchronize(const int idx = -1);

    static int program_running();

    static void* device_malloc(const std::size_t sz);

    static void device_free(void* ptr);

    static int isProgramRunning();

#ifdef AMREX_USE_CUDA
    template<typename T>
    static std::shared_ptr<T> create_host_pointer(const T* ptr, const int n = 1, const int m = -1);

    template<typename T>
    static std::shared_ptr<T> create_device_pointer(const int n = 1);
#endif

    static void device_htod_memcpy       (void* p_d, const void* p_h, const std::size_t sz);
    static void device_dtoh_memcpy       (void* p_h, const void* p_d, const std::size_t sz);
    static void device_htod_memcpy_async (void* p_d, const void* p_h, const std::size_t sz);
    static void device_dtoh_memcpy_async (void* p_h,const  void* p_d, const std::size_t sz);

    static void start_profiler();
    static void stop_profiler();

#if (defined(AMREX_USE_CUDA) && defined(__CUDACC__))
    static void c_comps_threads_and_blocks(const int* lo, const int* hi, const int comps, dim3& numBlocks, dim3& numThreads);

    static void n_threads_and_blocks(const long N, dim3& numBlocks, dim3& numThreads);

    static void c_threads_and_blocks(const int* lo, const int* hi, dim3& numBlocks, dim3& numThreads);

    // atmyers: We should switch parameters int& numThreads and int& numBlocks to be consistent.
    static void particle_threads_and_blocks(const int np, int& numThreads, int& numBlocks);

    static void grid_stride_threads_and_blocks(dim3& numBlocks, dim3& numThreads);

    static cudaStream_t stream_from_index (int idx);

    static void initialize_cuda_c ();

    static cudaStream_t cudaStream() { return cuda_stream; }

    static void setNumThreadsMin(int tx, int ty, int tz) { numThreadsMin.x = tx; numThreadsMin.y = ty; numThreadsMin.z = tz; return; }
#endif

    static void mem_advise_set_preferred(void* p, const std::size_t sz, const int device);

    static void mem_advise_set_readonly(void* p, const std::size_t sz);

private:

    static int device_id;

    static int verbose;

#if defined(AMREX_USE_CUDA) && defined(__CUDACC__)
    static const int max_cuda_streams = 16;
    static cudaStream_t cuda_streams[max_cuda_streams];
    static cudaStream_t cuda_stream;

    static dim3 numThreadsMin;

    static dim3 numBlocksOverride, numThreadsOverride;

    static cudaDeviceProp device_prop;
#endif
};

}}

#ifdef AMREX_USE_CUDA
template<typename T>
std::shared_ptr<T>
amrex::Cuda::Device::create_host_pointer(const T* ptr, const int n, const int m)
{
    std::shared_ptr<T> h_ptr;

    T* temp_ptr;
    const std::size_t sz = n * sizeof(T);
    gpu_hostalloc((void**) &temp_ptr, &sz);

    // Copy only m values if the argument was specified.

    int num;

    if (m < 0) {
        num = n;
    }
    else {
        num = m;
    }

    for (int i = 0; i < num; ++i) {
        *(temp_ptr + i) = *(ptr + i);
    }
    h_ptr.reset(temp_ptr, [](T* p) { gpu_freehost(p); });

    return h_ptr;
}
#endif



#ifdef AMREX_USE_CUDA
template<typename T>
std::shared_ptr<T>
amrex::Cuda::Device::create_device_pointer(int n)
{

    std::shared_ptr<T> d_ptr;

    T* temp_ptr = (T*) device_malloc(n * sizeof(T));

    d_ptr.reset(temp_ptr, [](T* p) { gpu_free(p); });

    return d_ptr;

}
#endif


extern "C" {
    void* amrex_gpu_malloc (std::size_t size);
    void amrex_gpu_free (void* p);
}

#endif
