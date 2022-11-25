FROM bitnami/minideb@sha256:f643a1ae18ea62acdc1d85d1892b41a0270faeb0e127c15e6afe41209d838b33

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
    apt-get install --no-install-recommends -y libstdc++6 libc6 libnsl2 && \
    rm -rf /var/lib/apt/lists/*


COPY kagome /usr/local/bin/
