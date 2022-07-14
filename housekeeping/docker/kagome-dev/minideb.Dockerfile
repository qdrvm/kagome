FROM bitnami/minideb:bullseye

MAINTAINER Vladimir Shcherba <abrehchs@gmail.com>

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
        curl && \
        rm -rf /var/lib/apt/lists/*

ENV LLVM_VERSION=11
ENV GCC_VERSION=9

# add repos for llvm and newer gcc and install docker
RUN curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg && \
    echo \
      "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/debian \
      bullseye stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null && \
    apt-get update && apt-get install --no-install-recommends -y \
        docker-ce \
        docker-ce-cli \
        containerd.io \
        build-essential \
        gcc-${GCC_VERSION} \
        g++-${GCC_VERSION} \
        llvm-${LLVM_VERSION}-dev \
        clang-${LLVM_VERSION} \
        clang-tidy-${LLVM_VERSION} \
        clang-format-${LLVM_VERSION} \
        make \
        git \
        ccache \
        lcov \
        zlib1g-dev \
        unzip && \
    rm -rf /var/lib/apt/lists/*

# install rustc
ENV RUST_VERSION=nightly-2022-06-10
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

# install cmake and dev dependencies
RUN python3 -m pip install --no-cache-dir --upgrade pip
RUN pip3 install --no-cache-dir cmake scikit-build requests gitpython gcovr pyyaml

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
ENV LLVM_ROOT=/usr/lib/llvm-${LLVM_VERSION}
ENV LLVM_DIR=/usr/lib/llvm-${LLVM_VERSION}/lib/cmake/llvm/
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-${GCC_VERSION}
ENV CXX=g++-${GCC_VERSION}

# set default compilers and tools
RUN update-alternatives --install /usr/bin/python       python       /usr/bin/python3                      90 && \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-${LLVM_VERSION}   90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${LLVM_VERSION} 90 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-${LLVM_VERSION}/bin/clang-${LLVM_VERSION} 90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-${LLVM_VERSION}      90 && \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-${GCC_VERSION}           90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-${GCC_VERSION}           90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-${GCC_VERSION}          90

RUN groupadd kagome \
  && useradd -g kagome -M --shell /bin/bash kagome

USER kagome
