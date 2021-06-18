This repo houses accompanying files for my postgraduate dissertation. The dissertation itself is available upon request.

This repo is organised as follows:

* **P3ARSEC** folder contains the source code of the PARSEC benchmark suite.
* **Rodinia-Streamcluster** folder contains the source code of the Streamcluster benchmark from the Rodinia benchmark suite.
* **BlackScholes-MPI** folder includes an MPI implementation of the Blackscholes benchmark.
* **Docker-experiments** folder contains DockerFiles of the images used to conduct experiments on Docker
* **Results** folder contains the results of the experiments that have been used in jupyter notebook files for analysis and generation of plots and charts that can be found in the dissertation.

##### ABSTRACT

Since the creation of parallel programming libraries, programmers have always preferred to code with libraries that improve their productivity and achieve the best performance without considering the energy footprint of their programs. However, the energy consumption of data centers keeps increasing, rising the need for finding trade-offs between application performance and energy consumption. Hence, this project aims to evaluate the performance and energy consumption of parallel programming libraries using the industry-standard PARSEC benchmark suite. The experiments cover commonly used parallel programming libraries, Pthreads and OpenMP, as well as high level parallel pattern libraries like FastFlow and TBB. The results show that solutions based on parallel patterns and implemented with libraries that handle low level details performs better and consumes less energy. Further, different combinations of parallel patterns can result in different performance and energy outputs which highlights the importance of design decisions in achieving better performance and energy trade-offs. The study also goes to show that back-end code generation with the skeleton programming library SkePU adds a little overhead in heterogeneous applications. Since often High Performance Computing infrastructures utilize containerization technologies like Docker, this project dedicates some experiments to show that Docker containers affect the performance and energy consumption of parallel applications. Overall, these results emphasize the importance of considering design decisions of parallel applications along with implementation and deployment decision to find the best trade-offs between performance and energy consumption.
