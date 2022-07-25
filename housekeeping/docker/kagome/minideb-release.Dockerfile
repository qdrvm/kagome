FROM bitnami/minideb@sha256:f643a1ae18ea62acdc1d85d1892b41a0270faeb0e127c15e6afe41209d838b33

WORKDIR /kagome
ENV PATH $PATH:/kagome

RUN apt-get update && \
    apt-get install -y --no-install-recommends software-properties-common && \
    add-apt-repository -y "deb http://deb.debian.org/debian stable main" && \
    apt-get update && \
    apt-get install --no-install-recommends -y libstdc++6 libc6 curl libnsl2 && \
    rm -rf /var/lib/apt/lists/*

COPY kagome /usr/local/bin/
COPY kagome-db-editor /usr/local/bin/
