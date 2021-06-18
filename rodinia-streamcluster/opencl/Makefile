include ../../common/make.config

#C compiler
CC = g++


FLAGS = -O3
SRC = streamcluster.cpp
EXE = streamcluster
EXTRA_FLAGS = -I/home/sd272/meterPU/meterpu_0.81 -I/home/sd272/pcm -L/home/sd272/pcm -Wall -DENABLE_PCM -DENABLE_NVML -lpcm -lpthread -I/usr/local/cuda-11.2/targets/x86_64-linux/include/ -L/usr/local/cuda-11.2/targets/x86_64-linux/include/lib/stubs/lib -lnvidia-ml

release:$(SRC)
	$(CC) -lOpenCL  $(SRC) -o $(EXE) -I$(OPENCL_INC) -L$(OPENCL_LIB)	$(FLAGS) $(EXTRA_FLAGS)

errmsg:$(SRC)
	$(CC) -lOpenCL $(SRC) -o $(EXE) -I$(OPENCL_INC) -L$(OPENCL_LIB) -D ERRMSG $(FLAGS) $(EXTRA_FLAGS)
	
ptx:$(SRC)
	$(CC) -lOpenCL $(SRC) -o $(EXE) -I$(OPENCL_INC) -L$(OPENCL_LIB) -D PTX_MSG $(FLAGS) $(EXTRA_FLAGS)
	
res:$(SRC)
	$(CC) -lOpenCL $(SRC) -o $(EXE) -I$(OPENCL_INC) -L$(OPENCL_LIB) -D RES_MSG $(FLAGS) $(EXTRA_FLAGS)
	
clean: $(SRC)
	rm -f $(EXE) $(EXE).linkinfo result*
