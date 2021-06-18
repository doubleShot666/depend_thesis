#include "OptionDataStruct.h"

__global__ void blackscholes_skepu_cuda_precompiled_MapKernel_mapFunction(float* skepu_output, OptionData_ *data_e,  size_t skepu_w2, size_t skepu_w3, size_t skepu_w4, size_t skepu_n, size_t skepu_base)
{
	size_t skepu_i = blockIdx.x * blockDim.x + threadIdx.x;
	size_t skepu_gridSize = blockDim.x * gridDim.x;
	

	while (skepu_i < skepu_n)
	{
		
		
		auto skepu_res = skepu_userfunction_skepu_skel_0map_mapFunction::CU(data_e[skepu_i]);
		skepu_output[skepu_i] = skepu_res;
		skepu_i += skepu_gridSize;
	}
}
