FROM bitnami/minideb@sha256:297209ec9579cf8a5db349d5d3f3d3894e2d4281ee79df40d479c16896fdf41e

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
    echo \
      "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-11 main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-12 main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-13 main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-14 main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-15 main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    echo \
      "deb http://deb.debian.org/debian/ testing main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    wget -q -O- https://github.com/rui314/mold/releases/download/v1.5.1/mold-1.5.1-x86_64-linux.tar.gz | tar -C /usr/local --strip-components=1 -xzf - && \
    apt-get update && apt-get install --no-install-recommends -y \
        docker-ce \
        docker-ce-cli \
        containerd.io \
        build-essential \
        gcc-10 \
        g++-10 \
        gcc-11 \
        g++-11 \
        gcc-12 \
        g++-12 \
        clang-11 \
        llvm-11-dev \
        clang-tidy-11 \
        clang-format-11 \
        clang-12 \
        llvm-12-dev \
        clang-tidy-12 \
        clang-format-12 \
        clang-13 \
        llvm-13-dev \
        clang-tidy-13 \
        clang-format-13 \
        clang-14 \
        llvm-14-dev \
        clang-tidy-14 \
        clang-format-14 \
        clang-15 \
        llvm-15-dev \
        clang-tidy-15 \
        clang-format-15 \
        make \
        git \
        ccache \
        lcov \
        zlib1g-dev \
        unzip && \
    rm -rf /var/lib/apt/lists/*

# install rustc
ENV RUST_VERSION=nightly-2022-11-20
ENV RUSTUP_HOME=/root/.rustup
ENV CARGO_HOME=/root/.cargo
ENV PATH="${CARGO_HOME}/bin:${PATH}"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}

# install cmake and dev dependencies
RUN python3 -m pip install --no-cache-dir --upgrade pip
RUN pip3 install --break-system-packages --no-cache-dir cmake scikit-build requests gitpython gcovr pyyaml

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
ENV LLVM_ROOT=/usr/lib/llvm-11
ENV LLVM_DIR=/usr/lib/llvm-11/lib/cmake/llvm/
ENV PATH=${LLVM_ROOT}/bin:${LLVM_ROOT}/share/clang:${PATH}
ENV CC=gcc-10
ENV CXX=g++-10

# set default compilers and tools
RUN update-alternatives --install /usr/bin/python       python       /usr/bin/python3               90 && \
    
    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-11         90 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-11       90 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-11/bin/clang-11  90 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-11            90 && \

    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-12         80 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-12       80 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-12/bin/clang-12  80 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-12            80 && \

    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-13         70 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-13       70 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-13/bin/clang-13  70 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-13            70 && \

    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-14         60 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-14       60 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-14/bin/clang-14  60 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-14            60 && \

    update-alternatives --install /usr/bin/clang-tidy   clang-tidy   /usr/bin/clang-tidy-15         50 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-15       50 && \
    update-alternatives --install /usr/bin/clang        clang        /usr/lib/llvm-15/bin/clang-15  50 && \
    update-alternatives --install /usr/bin/clang++      clang++      /usr/bin/clang++-15            50 && \

    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-10                90 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-10                90 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-10               90 && \

    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-11                80 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-11                80 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-11               80 && \

    update-alternatives --install /usr/bin/gcc          gcc          /usr/bin/gcc-12                70 && \
    update-alternatives --install /usr/bin/g++          g++          /usr/bin/g++-12                70 && \
    update-alternatives --install /usr/bin/gcov         gcov         /usr/bin/gcov-12               70
