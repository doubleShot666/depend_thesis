PREFIX=${PARSECDIR}/pkgs/kernels/streamcluster/inst/${PARSECPLAT}

TARGET_C = sc_cpu

TARGET_O = sc_omp

ifdef version
  ifeq "$(version)" "parallel"
    CXXFLAGS :=	$(CXXFLAGS) -DENABLE_THREADS -pthread
  endif
endif

all: cpu omp

cpu:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) streamcluster_original.cpp -o $(TARGET_C) $(LIBS) -DENABLE_THREADS -pthread

omp:
	g++ -O3 -fopenmp -o $(TARGET_O) streamcluster_omp.cpp -I/home/sd272/meterPU/meterpu_0.81 -I/home/sd272/pcm -L/home/sd272/pcm -Wall -DENABLE_PCM -DENABLE_NVML -lpcm -I/usr/local/cuda-11.2/targets/x86_64-linux/include/ -L/usr/local/cuda-11.2/targets/x86_64-linux/include/lib/stubs/lib -lnvidia-ml


clean:
	rm -f *.o *~ *.txt sc_cpu sc_omp
	
