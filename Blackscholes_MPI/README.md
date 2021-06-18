This code is the Distributed Memory implementation with MPI of the Blackscholes benchmark from The PARSEC benchmark suite. It has been integrated with SkePU implementation to provide heterogeneous and hybrid execution of the benchmark.

**Original Author:** Richard O. Lee

**Modified by:** Chirstian Bienia and Christian Fensch

**MPI implementation by:** Jahanzeb Maqbool, SEECS, NUST, Pakistan. (https://github.com/jahanzebmaqbool/parsec-mpi)

**Integration with SkePU implementation by :** Sami Djouadi, University of St andrews, UK


## Compilation of the version combining MPI and SkePU

#### SkePU with OpenMP back-end :

From _mpi_skepu_omp_ folder generate OpenMP code with SkePU tool
```
[Path to SkePU tool] -openmp blackschole_lib.hpp --dir=./ -name=blackscholes_skepu_openmp_precompiled -- -std=c++11 -I[Path to SkePU includer folder] -I[Path to Clang include folder]
```

Compile source code into executable

```
mpic++ blackscholes_skepu_openmp_precompiled.cpp blackscholes_mpi.cpp -I[Path to SkePU includer folder]  -I[Path to MeterPU installation] -I[Path to PCM installation] -L[Path to PCM installation] -Wall -DENABLE_PCM -DENABLE_NVML -lpcm -I[Path to CUDA include folder] -L[Path to CUDA library folder] -lnvidia-ml -fopenmp -o blackscholes_mpi_skepu_omp
```

#### SkePU with CUDA back-end:

From _gpu_ folder generate CUDA code with SkePU tool
```
[Path to SkePU tool] -cuda blackschole_lib_cuda.hpp --dir=./ -name=blackscholes_skepu_cuda_precompiled -- -std=c++11 -I[Path to SkePU includer folder] -I[Path to Clang include folder]
```

Compile generate code with nvcc
```
nvcc -c -O3 -std=c++11 -arch sm_52 -I[Path to OpenMPI include folder] -I[Path to SkePU includer folder] -m64 blackscholes_skepu_cuda_precompiled.cu
```

Compile C++ source code to object file
```
mpic++ -c blackscholes_mpi.cpp -I[Path to SkePU includer folder]  -I[Path to MeterPU installation] -I[Path to PCM installation] -L[Path to PCM installation] -Wall -DENABLE_PCM -DENABLE_NVML -lpcm -I[Path to CUDA include folder] -L[Path to CUDA library folder] -lnvidia-ml
```

Compile with nvcc object files generated from the last two commands
```
nvcc -lm  -lcudart -lcublas -L[Path to OpenMPI include folder] -lmpi_cxx -lmpi *.o -I[Path to MeterPU installation] -I[Path to PCM installation] -L[Path to PCM installation] -DENABLE_PCM -DENABLE_NVML -lpcm -I[Path to CUDA include folder] -L[Path to CUDA library folder] -lnvidia-ml -o blackscholes_mpi_skepu_cuda
```

## Compilation of the version combining MPI and OpenMP

```
mpic++ blackscholes_mpi.cpp -I[Path to MeterPU installation] -I[Path to PCM installation] -L[Path to PCM installation] -Wall -DENABLE_PCM -DENABLE_NVML -lpcm -I[Path to CUDA include folder] -L[Path to CUDA library folder] -lnvidia-ml -fopenmp -o blackscholes_mpi_omp
```

## Running the benchmark

On the host OS:
```
mpirun -np [number of processors] [the executable name] in_10M.txt out.txt
```

On docker containers
```
sudo docker exec --user mpirun --privileged [name of the master container] mpirun -n [number of workers] --allow-run-as-root --hostfile [hostfile of workers] [the executable name] [path to input file]/in_10M.txt [path to output file]/out.txt
```

