FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    zlib1g \
    libgcc-s1 \
    libc6 \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/local/bin

COPY ./build/tmp/test/polkadot /usr/local/bin/polkadot
COPY ./build/tmp/test/polkadot-execute-worker /usr/local/bin/polkadot-execute-worker
COPY ./build/tmp/test/polkadot-prepare-worker /usr/local/bin/polkadot-prepare-worker

RUN chmod +x /usr/local/bin/polkadot /usr/local/bin/polkadot-execute-worker /usr/local/bin/polkadot-prepare-worker

CMD ["polkadot"]
