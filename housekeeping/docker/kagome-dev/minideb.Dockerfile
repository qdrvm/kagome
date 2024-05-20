FROM bitnami/minideb@sha256:a0dd12fa3f8b98f82f6d9e71cf1b81d8fd50a03e44f152f0b2b876e544639ca5
MAINTAINER Vladimir Shcherba <abrehchs@gmail.com>

SHELL ["/bin/bash", "-c"]

ENV KAGOME_IN_DOCKER=1

# add some required tools
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        gpg \
        gpg-agent \
        wget \
        vim \
        python3 \
        python3-pip \
        python3-setuptools \
        software-properties-common \
        gdb \
        gdbserver \
        curl && \
        rm -rf /var/lib/apt/lists/*

# add repos for llvm and newer gcc and install docker
RUN curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg && \
    echo \
      "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/debian \
      bullseye stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-15 main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://deb.debian.org/debian/ testing main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    wget -q -O- https://github.com/rui314/mold/releases/download/v1.5.1/mold-1.5.1-x86_64-linux.tar.gz | tar -C /usr/local --strip-components=1 -xzf - && \
    apt-get update && apt-get install --no-install-recommends -y \
        build-essential \
        gcc-12 \
        g++-12 \
        llvm-16-dev \
        clang-tidy-16 \
        clang-format-16 \
        libclang-rt-16-dev \
        make \
        git \
        ccache \
        lcov \
        zlib1g-dev \
        libgmp10 \
        libnsl-dev \
        libseccomp-dev \
        unzip && \
    rm -rf /var/lib/apt/lists/*

# install rustc
ENV RUST_VERSION=1.77.0
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}


# Prepare python venv
RUN apt update && \
    apt install --no-install-recommends -y \
    python3.11-venv
RUN python3 -m venv /venv

# install cmake and dev dependencies
RUN /venv/bin/python3 -m pip install --no-cache-dir --upgrade pip
RUN /venv/bin/pip install --no-cache-dir cmake==3.25 scikit-build requests gitpython gcovr pyyaml

ENV HUNTER_PYTHON_LOCATION=/venv/bin/python3

# set env
ENV LLVM_ROOT=/usr/lib/llvm-16
ENV LLVM_DIR=/usr/lib/llvm-16/lib/cmake/llvm/
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-12
ENV CXX=g++-12

# set default compilers and tools

RUN update-alternatives --install /usr/bin/python       python       /venv/bin/python3              90 && \
    update-alternatives --install /usr/bin/python       python       /usr/bin/python3               80 && \
    \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-16         50 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-16       50 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-16/bin/clang-16  50 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-16            50 && \
    \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-12                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-12                90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-12               90

# Install Docker
RUN apt-get install --no-install-recommends -y \
            docker-ce \
            docker-ce-cli \
            containerd.io