FROM bitnami/minideb@sha256:1cc3df6a4098088cc6d750ad3ce39ea2d169a19a619f82f49dbcf3ad55ab7b4b

WORKDIR /kagome
ENV PATH $PATH:/kagome

RUN apt-get update && \
    apt-get install -y --no-install-recommends software-properties-common curl gpg gpg-agent wget && \
    curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg && \
    echo \
      "deb http://deb.debian.org/debian/ experimental main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository -y "deb http://deb.debian.org/debian/ testing main" && \
    apt-get update && \
    apt-get install --no-install-recommends -y libgmp10 libstdc++6 libc6 libnsl2 libatomic1 gdb gdbserver && \
    rm -rf /var/lib/apt/lists/*

COPY kagome /usr/local/bin/
COPY libwasmedge.so* /usr/local/lib/
