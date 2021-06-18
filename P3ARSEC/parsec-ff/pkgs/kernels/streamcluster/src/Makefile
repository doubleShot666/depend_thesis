# Makefile for streamcluster

PREFIX=${PARSECDIR}/pkgs/kernels/streamcluster/inst/${PARSECPLAT}

COMPILER= C
SKEPU_TOOL=/home/sd272/skepu_installation/usr/local/bin/skepu-tool
SKEPU_INCLUDE_PATH=/home/sd272/skepu_installation/usr/local/include
CLANG_PATH=/usr/lib/clang/3.4.2/include
PCM_PATH=/home/sd272/pcm
NVCC=nvcc
CUDAFLAGS= -I${SKEPU_INCLUDE_PATH} -DENABLE_PARSEC_HOOKS -I${PARSECDIR}/pkgs/libs/hooks/inst/${PARSECPLAT}/include -I${MAMMUT_PATH}

TARGET=streamcluster
OBJS=streamcluster.o

ifdef version
  ifeq "$(version)" "pthreads"
    CXXFLAGS :=	$(CXXFLAGS) -DENABLE_THREADS -pthread
    OBJS += parsec_barrier.o
  endif
  ifeq "$(version)" "tbb"
    CXXFLAGS := $(CXXFLAGS) -DTBB_VERSION
  endif
  ifeq "$(version)" "ff"
    CXXFLAGS :=	$(CXXFLAGS) -DFF_VERSION -pthread
  endif
  ifeq "$(version)" "skepu2"
    ifdef variant
      ifeq "$(variant)" "cuda"
        COMPILER	= CUDA
        LDFLAGS=-L${PARSECDIR}/pkgs/libs/hooks/inst/${PARSECPLAT}/lib -L${MAMMUT_PATH}/mammut
        LIBS=-lhooks -lm -lmammut -lrt
      else ifeq "$(variant)" "hybrid"
		SRC			= streamcluster_skepu_hybrid.cpp
        CSRC		= streamcluster_skepu_hybrid_precompiled.cu
        LIBS=-lhooks -lm -lmammut -lrt
        COMPILER	= HYBRID
      else ifeq "$(variant)" "openmp"
        CXXFLAGS :=	$(CXXFLAGS)
        OBJS := streamcluster_skepu_omp.o
      endif
    endif
  endif
endif

ifeq ($(COMPILER),CUDA)
all: compile_cuda
else ifeq ($(COMPILER),HYBRID)
all: compile_hybrid
else
all: compile_c
endif

compile_hybrid:
	$(SKEPU_TOOL) --openmp --cuda $(SRC) --dir=./ -name=streamcluster_skepu_hybrid_precompiled -fnames=exp,log -- -std=c++11 -I$(SKEPU_INCLUDE_PATH) -I/usr/lib/clang/3.4.2/include --verbose
	$(NVCC) ./streamcluster_skepu_hybrid_precompiled.cu -Xcompiler -fopenmp -o $(TARGET) $(CUDAFLAGS) --linker-options -lgomp $(LDFLAGS) $(LIBS) -I$(SKEPU_INCLUDE_PATH) -L$(PCM_PATH) -lpcm

compile_cuda: clean
	$(NVCC) -c ./streamcluster_skepu_cuda_precompiled_2.cu $(CUDAFLAGS) --linker-options $(LDFLAGS) $(LIBS) -o $(TARGET)

compile_c: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGET)

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

