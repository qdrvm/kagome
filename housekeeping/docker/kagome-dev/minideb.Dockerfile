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
        gcc-9 \
        g++-9 \
        llvm-10-dev \
        clang-10 \
        clang-tidy-10 \
        clang-format-10 \
        make \
        git \
        ccache \
        lcov \
        zlib1g-dev \
        unzip && \
    rm -rf /var/lib/apt/lists/*

# install rustc
ENV RUST_VERSION=nightly-2021-10-04
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
ENV LLVM_ROOT=/usr/lib/llvm-10
ENV LLVM_DIR=/usr/lib/llvm-10/lib/cmake/llvm/
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-9
ENV CXX=g++-9

# set default compilers and tools
RUN update-alternatives --install /usr/bin/python       python       /usr/bin/python3               90 && \
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-10         90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-10       90 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-10/bin/clang-10  90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-10            90 && \
    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-9                 90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-9                 90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-9                90
