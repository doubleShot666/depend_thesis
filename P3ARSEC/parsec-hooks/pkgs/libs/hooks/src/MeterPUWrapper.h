//
// Created by Administrator on 3/15/2021.
//

#ifndef TEST_RUN_ON_SSH_METERPUWRAPPER_H
#define TEST_RUN_ON_SSH_METERPUWRAPPER_H

#if defined(__cplusplus)
extern "C" {
#endif

void* MeterPU_PCM_Energy_create();
void MeterPU_PCM_Energy_release(void* myclass);
void MeterPU_PCM_Energy_start(void* myclass);
void MeterPU_PCM_Energy_stop(void* myclass);
void MeterPU_PCM_Energy_calc(void* myclass);
double MeterPU_PCM_Energy_getValue(void* myclass);

void* MeterPU_NVML_Energy_create();
void MeterPU_NVML_Energy_release(void* myclass);
void MeterPU_NVML_Energy_start(void* myclass);
void MeterPU_NVML_Energy_stop(void* myclass);
void MeterPU_NVML_Energy_calc(void* myclass);
double MeterPU_NVML_Energy_getValue(void* myclass);

void* MeterPU_CUDA_Time_create();
void MeterPU_CUDA_Time_release(void* myclass);
void MeterPU_CUDA_Time_start(void* myclass);
void MeterPU_CUDA_Time_stop(void* myclass);
void MeterPU_CUDA_Time_calc(void* myclass);
double MeterPU_CUDA_Time_getValue(void* myclass);

#if defined(__cplusplus)
}
#endif

#endif //TEST_RUN_ON_SSH_METERPUWRAPPER_H
