PREFIX=${PARSECDIR}/pkgs/apps/blackscholes/inst/${PARSECPLAT}
SKEPU_TOOL=/home/sd272/skepu_installation/usr/local/bin/skepu-tool
SKEPU_INCLUDE_PATH=/home/sd272/skepu_installation/usr/local/include
CLANG_PATH=/usr/lib/clang/3.4.2/include
PCM_PATH=/home/sd272/pcm

#e
CSRC    = $(SRC)
TARGET  = blackscholes
M4_BASE = .
MACROS  = c.m4.pthreads
COMPILER= C

NVCC=nvcc
CUDAFLAGS= -I${SKEPU_INCLUDE_PATH} -DENABLE_PARSEC_HOOKS -I${PARSECDIR}/pkgs/libs/hooks/inst/${PARSECPLAT}/include -I${MAMMUT_PATH}



ifdef source
	ifeq "$(source)" "simd"
		SRC = blackscholes.simd.c
		CXXFLAGS += -msse3
	endif
else
	SRC	= blackscholes.c
endif

ifdef version
  ifeq "$(version)" "pthreads"
    M4_SRC    = blackscholes.m4.cpp
    CSRC      = $(M4_SRC)
    MT        = -DENABLE_THREADS
    CXXFLAGS += -pthread
  endif
  ifeq "$(version)" "openmp"
    MT        = -DENABLE_OPENMP -fopenmp
  endif
  ifeq "$(version)" "tbb"
    MT        = -DENABLE_TBB
  endif
  ifeq "$(version)" "ff"
    MT        = -DENABLE_FF
  endif
  ifeq "$(version)" "caf"
    MT        = -DENABLE_CAF
  endif
  ifeq "$(version)" "skepu"
    #./skepu-tool-openmp blackscholes_skepu.cpp -name=blackscholes_skepu_omp -fnames=exp,log -- -std=c++11 -I../lib/Headers -I/tmp/p3arsec/pkgs/libs/skepu2/include -I/usr/include
    ifdef variant
      ifeq "$(variant)" "cuda"
        SRC			= blackscholes_skepu_cuda.cpp
        COMPILER	= CUDA
        LDFLAGS=-L${PARSECDIR}/pkgs/libs/hooks/inst/${PARSECPLAT}/lib -L${MAMMUT_PATH}/mammut
        LIBS=-lhooks -lm -lmammut -lrt
      else ifeq "$(variant)" "hybrid"
      	SRC			= blackscholes_skepu_hybrid.cpp
      	CSRC		= blackscholes_skepu_hybrid_precompiled.cu
        LIBS=-lhooks -lm -lmammut -lrt
      	COMPILER	= HYBRID
      else ifeq "$(variant)" "openmp"
        CSRC = blackscholes_skepu_omp.cpp
        MT = -fopenmp
      endif
    endif
  endif
  ifeq "$(version)" "nornir"
    MT        = -DENABLE_NORNIR_NATIVE
  endif
else
  MT        =
endif



# Default build single precision version
NCO     = -DNCO=4

ifdef chk_err
ERR     = -DERR_CHK
endif

ifdef single
NCO = -DNCO=4
endif

ifdef size
SZ = -DN=$(size)
else
SZ = -DN=960
endif

ifdef double
NCO = -DNCO=2
endif

CXXFLAGS += $(MT) $(NCO) $(FUNC) $(ERR) $(CSRC)

#all: #$(TARGET)

ifeq ($(COMPILER),CUDA)
all: compile_cuda
else ifeq ($(COMPILER),HYBRID)
all: compile_hybrid
else
all: compile_c
endif

compile_hybrid:
	$(SKEPU_TOOL) --openmp --cuda $(SRC) --dir=./ -name=blackscholes_skepu_hybrid_precompiled -fnames=exp,log -- -std=c++11 -I$(SKEPU_INCLUDE_PATH) -I$(CLANG_PATH) --verbose
	$(NVCC) ./blackscholes_skepu_hybrid_precompiled.cu -Xcompiler -fopenmp -o $(TARGET) $(CUDAFLAGS) --linker-options -lgomp $(LDFLAGS) $(LIBS) -I$(SKEPU_INCLUDE_PATH) -L$(PCM_PATH) -lpcm

compile_c: clean $(CSRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o $(TARGET) -L$(PCM_PATH) -lpcm

$(M4_SRC): $(SRC)
	$(M4) $(M4_BASE)/$(MACROS) $< > $@

compile_cuda: clean generate
	$(NVCC) ./blackscholes_skepu_cuda_precompiled.cu -o $(TARGET) $(CUDAFLAGS) --linker-options $(LDFLAGS) $(LIBS)

generate: $(SRC)
	$(SKEPU_TOOL) --cuda $(SRC) --dir=./ -name=blackscholes_skepu_cuda_precompiled -fnames=exp,log -- -std=c++11 -I$(SKEPU_PATH) -I $(CLANG_PATH) --verbose

clean:
	rm -f $(TARGET) $(M4_SRC) ./blackscholes_skepu_cuda_precompiled.cu

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

