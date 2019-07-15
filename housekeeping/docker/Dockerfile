FROM ubuntu:18.04

MAINTAINER Bohdan Vanieiev <bogdan@soramitsu.co.jp>

# add llvm repo
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        gpg \
        gpg-agent \
        wget \
        software-properties-common \
        curl \
        python3 \
        python3-pip \
        python3-setuptools \
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository -y "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main" && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test

# install cmake and dev dependencies
RUN pip3 install --upgrade pip cmake requests gitpython gcovr pyyaml

# install rustc
ENV RUST_VERSION=nightly-2019-07-07
ENV PATH="/root/.cargo/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

# install dev dependencies
RUN apt-get update && apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
        xz-utils \
        build-essential \
        gcc-8 \
        g++-8 \
        llvm-8 \
        clang-8 \
        clang-tidy-8 \
        clang-format-8 \
        git \
        ccache \
        lcov \
        ninja-build \
        vim \
    && rm -rf /var/lib/apt/lists/*

# set env
ENV LLVM_ROOT=/usr/lib/llvm-8
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=clang-8
ENV CXX=clang++-8

# set default compilers and tools
RUN update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-8               90 && \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-8                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-8                90 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-8/bin/clang-8   90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-8            90 && \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-8         90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-8       90
