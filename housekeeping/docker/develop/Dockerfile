FROM ubuntu:20.04

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
    add-apt-repository -y "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-10 main" && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && rm -rf /var/lib/apt/lists/*

# install cmake and dev dependencies
RUN python3 -m pip install --no-cache-dir --upgrade pip
RUN pip3 install --no-cache-dir scikit-build cmake requests gitpython gcovr pyyaml

# install rustc
ENV RUST_VERSION=nightly-2019-07-07
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

# install dev dependencies
RUN apt-get update && apt-get upgrade -y && \
    apt-get install --no-install-recommends -y \
        xz-utils \
        build-essential \
        gcc-8 \
        g++-8 \
        gcc-9 \
        g++-9 \
        gcc-10 \
        g++-10 \
        llvm-8 \
        clang-8 \
        clang-tidy-8 \
        clang-format-8 \
        git \
        ccache \
        lcov \
        ninja-build \
        vim \
        unzip\
        docker.io \
    && rm -rf /var/lib/apt/lists/*

# install sonar cli
ENV SONAR_CLI_VERSION=4.1.0.1829
RUN set -e; \
    mkdir -p /opt/sonar; \
    curl -L -o /tmp/sonar.zip https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${SONAR_CLI_VERSION}-linux.zip; \
    unzip -o -d /tmp/sonar-scanner /tmp/sonar.zip; \
    mv /tmp/sonar-scanner/sonar-scanner-${SONAR_CLI_VERSION}-linux /opt/sonar/scanner; \
    ln -s -f /opt/sonar/scanner/bin/sonar-scanner /usr/local/bin/sonar-scanner; \
    rm -rf /tmp/sonar*

# set env
ENV LLVM_ROOT=/usr/lib/llvm-8
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-8
ENV CXX=g++-8

# set default compilers and tools
RUN update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-8               90 && \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-8                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-8                90 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-8/bin/clang-8   90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-8            90 && \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-8         90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-8       90 && \
    update-alternatives --install /usr/bin/python       python       /usr/bin/python3              90
