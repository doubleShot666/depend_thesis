This repository was forked from P<sup>3</sup>ARSEC repository (https://github.com/ParaGroup/p3arsec). Here we highlight the most important points from the documentation.

This repository contains parallel patterns implementations of some applications contained in the PARSEC benchmark.

The structure and modelling of the applications is described in the paper:

[**Bringing Parallel Patterns out of the Corner: the P<sup>3</sup>ARSEC Benchmark Suite**</br>
Daniele De Sensi, Tiziano De Matteis, Massimo Torquati, Gabriele Mencagli, Marco Danelutto</br>
*ACM Transactions on Architecture and Code Optimization (TACO), October 2017* </br>](https://dl.acm.org/citation.cfm?id=3132710)

[Release v1.0](https://github.com/paragroup/p3arsec/releases/tag/v1.0) was used in the paper.


# Compile
To let PARSEC properly work, some dependencies needs to be installed. For Ubuntu systems, you can do it with the following command:
```
sudo apt-get install git build-essential m4 x11proto-xext-dev libglu1-mesa-dev libxi-dev libxmu-dev libtbb-dev libssl-dev
```

For Arch Linux, the following:

```
sudo pacman -Sy git m4 xorgproto glu libxi libxmu intel-tbb openssl
```

Similar packages can be found for other Linux distributions.

After that, you need to install the benchmarks you are interested in:

```
cd bin
```
The full documentation on how to use the script (`./parsecmgmt`) can be found [here](http://parsec.cs.princeton.edu/doc/man/man1/parsecmgmt.1.html).



To compile the parallel patterns version of a specific benchmark, is sufficient to run the following command:
```
./parsecmgmt -a build -p [BenchmarkName] -c gcc-[library]
```

The available parallel programming libraries depend on the benchmark, here are all possible options:

* *skepu* for the SkePU3 parallel pattern-based implementation. The parameter *-v [openmp|cuda|hybrid]* should be added
* *pthreads* for the Pthreads implementation.
* *openmp* for the OpenMP implementation.
* *tbb* for the Intel TBB implementation.
* *ff* for the FastFlow implementation

# Run
Once you compiled a benchmark, you can run it with:
```
./parsecmgmt -a run -p [BenchmarkName] -c gcc-[library] -n [ConcurrencyLevel] -i [dataset]
```
The data sets are downloaded by running the `./install.sh` script. 

By default, the program is run on a test input. PARSEC provides different input datasets: *test*, 
*simsmall*, *simmedium*, *simlarge*, *simdev* and *native*.
The *native* input set is the one resembling a real execution scenario, while the others should be used for testing/simulation
purposes.


The *concurrency level* is is the *minimum* number of threads that will be activated by the application. 
Accordingly, for example there are the following values:

* *blackscholes*: n+1 threads.
* *ferret*:  n threads for each pipeline stage (4n + 4 threads). (For the *pipe of farms* version.)


## Measuring time and energy consumption
The benchmark should run with `sudo` which required for energy and time measurement libraries [Mammut library](http://danieledesensi.github.io/mammut/), and [Meter PU](https://github.com/lilu09/meterpu)

The measurements target the Region Of Interest where is the parallel part of the application, excluding initialisation and cleanup phases.
Time measures are displayed in seconds and energy consumption in joules.

Energy measurements cover CPU cores, DRAM, and GPU, energy consumption which are collected and displayed in separate values.


# Run alternative versions
*ferret* has been implemented according to different pattern compositions.
To run versions different from the default one, you need first to remove the existing one (if present).
To do so, execute:

```
./parsecmgmt -a fullclean -c gcc-ff -p [BenchmarkName]
./parsecmgmt -a fulluninstall -c gcc-ff -p [BenchmarkName]
```

To compile and run the other versions, please refer to the following sections.
## Ferret
At line 78 of the in the MakeFile of the Ferret benchmark, replace `ferret-ff-pipeoffarms` with:

* `ferret-ff-farm` if you want to run the *farm* version.
* `ferret-ff-farm-optimized` if you want to run the *farm (optimized)* version.
* `ferret-ff-farmofpipes` if you want to run the *farm of pipelines* version.
* `ferret-ff-pipeoffarms` if you want to run the *pipelines of farms* version.

After that, build and run *ferret* as usual.

# Contributors
P<sup>3</sup>ARSEC has been developed by [Daniele De Sensi](mailto:d.desensi.software@gmail.com) and [Tiziano De Matteis](mailto:dematteis@di.unipi.it).

The integration of [Meter PU](https://github.com/lilu09/meterpu) and the addition of GPU and Hybrid computing in the Blacksholes application was made by [Sami Djouadi](mailto:sd272@st-andrews.ac.uk)
