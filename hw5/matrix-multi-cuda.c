/*
Perform matrix multiplication on 2 2-D matrices using CUDA non-tiled
*/
#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>
#include <driver_types.h>
#include <time.h>
#include <sys/time.h>

__global__ void matrix_multi_kernel(int *m, int *n, int *p, int width);

void cuda_measure_start(cudaEvent_t *start, cudaEvent_t *stop);

void cuda_measure_stop(cudaEvent_t *start, cudaEvent_t *stop, float *elapsed_time_ms);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage %s matrix-width gird_width block_width\n", argv[0]);
        exit(1);
    }

    int *H_A, *H_B, *H_C;
    int *D_A, *D_B, *D_C;
    int width = atoi(argv[1]);
    int grid_width = atoi(argv[2]);
    int block_width = atoi(argv[3]);
    int MA_value = 3;
    int MB_value = 2;

    int N = width * width;
    int size = N * sizeof(int);

    if ((block_width * block_width * grid_width * grid_width) < (width * width))
    {
        printf("Error block_width^2 x grid_width^2 < width^2, try again!\n");
        return 1;
    }

    printf("Perform Matrix Multiplication on [%d x %d] x [%d x %d]\n", width, width, width, width);

    // dynamically allocate mem
    H_A = (int *)malloc(size);
    H_B = (int *)malloc(size);
    H_C = (int *)malloc(size);

    // initialize matrix A and B on the host
    for (int i = 0; i < N; ++i)
    {
        H_A[i] = MA_value;
        H_B[i] = MB_value;
    }

    dim3 dim_grid(grid_width, grid_width, 1);
    dim3 dim_block(block_width, block_width, 1);
    cudaEvent_t start, stop; // using cuda events to measure time
    float elapsed_time_ms;

    cuda_measure_start(&start, &stop);
    // ------------------------- start timing --------------------------- //

    // Allocate device memory and copy to device
    cudaMalloc((void **)&D_A, size);
    cudaMemcpy(D_A, H_A, size, cudaMemcpyHostToDevice);
    cudaMalloc((void **)&D_B, size);
    cudaMemcpy(D_B, H_B, size, cudaMemcpyHostToDevice);
    cudaMalloc((void **)&D_C, size);

    // CUDA kernel execution
    matrix_multi_kernel<<<dim_grid, dim_block>>>(D_A, D_B, D_C, width);

    // Copy 2d array from device back to host
    cudaMemcpy(H_C, D_C, size, cudaMemcpyDeviceToHost);

    // Wait for GPU to finish before accessing on host
    cudaDeviceSynchronize();

    // ------------------------- end timing --------------------------- //
    cuda_measure_stop(&start, &stop, &elapsed_time_ms);

    printf("Using CUDA:\n    time to calculate: %f ms.\n", elapsed_time_ms);

    // Check for errors (all values should be 3.0f)
    printf("Checking for error...\n");
    int max_error = 0;
    int target_value = MA_value * MB_value * width;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < width; j++)
        {
            max_error = fmax(max_error, abs(H_C[i * width + j] - target_value));
        }
    }
    printf("Max error: %d \n\n", max_error);

    // Free memory
    free(H_A);
    free(H_B);
    free(H_C);
    cudaFree(D_A);
    cudaFree(D_B);
    cudaFree(D_C);

    return 0;
}

/**
 * perform matrix multiplication using CUDA non-tiled
 */
__global__ void matrix_multi_kernel(int *m, int *n, int *p, int width)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if ((row < width) && (col < width))
    {
        int p_value = 0;

        for (int i = 0; i < width; ++i)
        {
            p_value += m[row * width + i] * n[i * width + col];
        }
        p[row * width + col] = p_value;
    }
}

void cuda_measure_start(cudaEvent_t *start, cudaEvent_t *stop)
{
    cudaEventCreate(start);
    cudaEventCreate(stop);
    cudaEventRecord(*start, 0);
}

void cuda_measure_stop(cudaEvent_t *start, cudaEvent_t *stop, float *elapsed_time_ms)
{
    cudaEventRecord(*stop, 0); // instrument code to measue end time
    cudaEventSynchronize(*stop);
    cudaEventElapsedTime(elapsed_time_ms, *start, *stop);

    cudaEventDestroy(*start);
    cudaEventDestroy(*stop);
}
