FROM bitnami/minideb:buster

MAINTAINER Vladimir Shcherba <abrehchs@gmail.com>

# add some required tools
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
        gpg \
        gpg-agent \
        vim \
        python3 \
        python3-pip \
        python3-setuptools \
        software-properties-common \
        curl && \
        rm -rf /var/lib/apt/lists/*

# add repos for llvm and newer gcc and install docker
RUN curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg && \
    echo \
      "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/debian \
      buster stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null && \
    add-apt-repository -y "deb http://deb.debian.org/debian testing main" && \
    apt-get update && apt-get install --no-install-recommends -y \
     docker-ce \
     docker-ce-cli \
     containerd.io \
     gcc-9 \
     g++-9 \
     make \
     cmake \
     git \
     ccache \
     lcov \
     unzip \
    && \
    rm -rf /var/lib/apt/lists/*

# install rustc
ENV RUST_VERSION=nightly-2020-09-24
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

# install cmake and dev dependencies
# RUN python3 -m pip install --no-cache-dir --upgrade pip
# RUN pip3 install --no-cache-dir cmake # scikit-build requests gitpython gcovr pyyaml

# set env
ENV CC=gcc-9
ENV CXX=g++-9

# set default compilers and tools
RUN update-alternatives --install /usr/bin/python       python       /usr/bin/python3              90 && \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-9                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-9                90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-9               90
