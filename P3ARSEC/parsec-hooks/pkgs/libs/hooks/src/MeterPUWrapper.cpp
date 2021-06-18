//
// Created by Sami on 3/15/2021.
//
#include <MeterPU.h>
#include "MeterPUWrapper.h"
using namespace MeterPU;

/*extern "C"*/ void* MeterPU_PCM_Energy_create() {
    return new Meter<PCM_Energy>;
}
/*extern "C"*/ void MeterPU_PCM_Energy_release(void* myclass) {
    delete static_cast<Meter<PCM_Energy>*>(myclass);
}

/*extern "C"*/ void MeterPU_PCM_Energy_start(void* myclass) {
    static_cast<Meter<PCM_Energy>*>(myclass)->start();
}

/*extern "C"*/ void MeterPU_PCM_Energy_stop(void* myclass) {
    static_cast<Meter<PCM_Energy>*>(myclass)->stop();
}

/*extern "C"*/ void MeterPU_PCM_Energy_calc(void* myclass) {
    static_cast<Meter<PCM_Energy>*>(myclass)->calc();
}

/*extern "C"*/ double MeterPU_PCM_Energy_getValue(void* myclass) {
    return static_cast<Meter<PCM_Energy>*>(myclass)->get_value();
}



/*extern "C"*/ void* MeterPU_NVML_Energy_create() {
    return new Meter<NVML_Energy<>>;
}
/*extern "C"*/ void MeterPU_NVML_Energy_release(void* myclass) {
    delete static_cast<Meter<NVML_Energy<>>*>(myclass);
}

/*extern "C"*/ void MeterPU_NVML_Energy_start(void* myclass) {
    static_cast<Meter<NVML_Energy<>>*>(myclass)->start();
}

/*extern "C"*/ void MeterPU_NVML_Energy_stop(void* myclass) {
    static_cast<Meter<NVML_Energy<>>*>(myclass)->stop();
}

/*extern "C"*/ void MeterPU_NVML_Energy_calc(void* myclass) {
    static_cast<Meter<NVML_Energy<>>*>(myclass)->calc();
}

/*extern "C"*/ double MeterPU_NVML_Energy_getValue(void* myclass) {
    return static_cast<Meter<NVML_Energy<>>*>(myclass)->get_value();
}



/*extern "C"*/ void* MeterPU_CUDA_Time_create() {
    return new Meter<CUDA_Time>;
}
/*extern "C"*/ void MeterPU_CUDA_Time_release(void* myclass) {
    delete static_cast<Meter<CUDA_Time>*>(myclass);
}

/*extern "C"*/ void MeterPU_CUDA_Time_start(void* myclass) {
    static_cast<Meter<CUDA_Time>*>(myclass)->start();
}

/*extern "C"*/ void MeterPU_CUDA_Time_stop(void* myclass) {
    static_cast<Meter<CUDA_Time>*>(myclass)->stop();
}

/*extern "C"*/ void MeterPU_CUDA_Time_calc(void* myclass) {
    static_cast<Meter<CUDA_Time>*>(myclass)->calc();
}

/*extern "C"*/ double MeterPU_CUDA_Time_getValue(void* myclass) {
    return static_cast<Meter<CUDA_Time>*>(myclass)->get_value();
}



