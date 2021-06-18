ARG CUDA_VERSION=11.2.0
ARG OS_VERSION=16.04
FROM nvidia/cuda:${CUDA_VERSION}-devel-ubuntu${OS_VERSION}

LABEL maintainer="sd272@st-andrews.ac.uk"



# Install package dependencies
RUN apt-get update -qq \
      && apt-get -y --no-install-recommends install \
         build-essential \
         ca-certificates \
         gdb \
         gfortran \
         wget \
      && apt-get clean all \
      && rm -r /var/lib/apt/lists/*


### Build OPENMPI ###

ARG OPENMPI_VERSION="4.0.2"
ARG OPENMPI_CONFIGURE_OPTIONS="--enable-fast=all,O3 --prefix=/usr --with-cuda=/usr/local/cuda"
ARG OPENMPI_MAKE_OPTIONS="-j4"

RUN mkdir -p /tmp/openmpi-build \
      && cd /tmp/openmpi-build \
      && OPENMPI_VER_MM="${OPENMPI_VERSION%.*}" \
      && wget http://www.openmpi.org/software/ompi/v${OPENMPI_VER_MM}/downloads/openmpi-${OPENMPI_VERSION}.tar.bz2 \
      && tar xjf openmpi-${OPENMPI_VERSION}.tar.bz2 \
      && cd openmpi-${OPENMPI_VERSION}  \
      && ./configure ${OPENMPI_CONFIGURE_OPTIONS} \
      && make ${OPENMPI_MAKE_OPTIONS} \
      && make install \
      && ldconfig \
      && cd / \
      && rm -rf /tmp/openmpi-build


### Final settings ###
WORKDIR /
CMD ["/bin/bash"]