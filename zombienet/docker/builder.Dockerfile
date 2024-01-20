# Polkadot docker image to fetch
ARG POLKADOT_RELEASE_GLOBAL
ARG POLKADOT_RELEASE_GLOBAL_NUMERIC
ARG ZOMBIENET_RELEASE

# Base image with all dependencies
FROM node:18-bullseye-slim as zombie-builder
USER root

RUN apt-get update && apt update && \
    apt-get install -y --no-install-recommends git ssh curl nano build-essential protobuf-compiler libprotobuf-dev time && \
    apt install -y --no-install-recommends clang ca-certificates openssl && \
    rm -rf /var/lib/apt/lists/* && \
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
ENV RUSTUP_HOME=/opt/rust CARGO_HOME=/opt/cargo PATH=/root/.cargo/bin:$PATH
RUN rustup default stable && rustup target add wasm32-unknown-unknown && rustup component add rust-src
RUN groupadd --gid 10001 nonroot && \
    useradd  --home-dir /home/nonroot \
             --create-home \
             --shell /bin/bash \
             --gid nonroot \
             --groups nonroot \
             --uid 10000 nonroot
WORKDIR /home/nonroot/

# Image with repository
FROM zombie-builder AS zombie-builder-polkadot-sdk
ARG POLKADOT_SDK_RELEASE
WORKDIR /home/nonroot/
RUN git clone --depth 1 --branch $POLKADOT_SDK_RELEASE https://github.com/paritytech/polkadot-sdk.git

# Image with test-parachain-adder-collator binary
FROM zombie-builder-polkadot-sdk AS zombie-builder-polkadot-sdk-bin
WORKDIR /home/nonroot/polkadot-sdk/
RUN cargo update -p test-parachain-adder-collator \
    -p polkadot-test-malus \
    -p test-parachain-undying-collator
RUN cargo build --profile testnet -p test-parachain-adder-collator \
    -p polkadot-test-malus \
    -p test-parachain-undying-collator
RUN find /home/nonroot/polkadot-sdk/target/ -maxdepth 2 -print
# RUN time cargo build --release -p polkadot-parachain-bin
# RUN time cargo build --release --bin polkadot
# test-parachain

# Image with polkadot
FROM docker.io/parity/polkadot:$POLKADOT_RELEASE_GLOBAL AS polkadot

FROM docker.io/parity/polkadot-parachain:$POLKADOT_RELEASE_GLOBAL_NUMERIC AS polkadot-parachain

FROM zombie-builder AS final

RUN mkdir -p /home/nonroot/bin
COPY --from=zombie-builder-polkadot-sdk-bin /home/nonroot/polkadot-sdk/target/testnet/malus /home/nonroot/bin
COPY --from=zombie-builder-polkadot-sdk-bin /home/nonroot/polkadot-sdk/target/testnet/adder-collator /home/nonroot/bin
COPY --from=zombie-builder-polkadot-sdk-bin /home/nonroot/polkadot-sdk/target/testnet/undying-collator /home/nonroot/bin
COPY --from=polkadot /usr/bin/polkadot /home/nonroot/bin
COPY --from=polkadot-parachain /usr/local/bin/polkadot-parachain /home/nonroot/bin
ENV PATH=/home/nonroot/bin:$PATH

RUN apt-get update && \
    apt-get install -y --no-install-recommends software-properties-common curl gpg gpg-agent wget && \
    curl -fsSL https://download.docker.com/linux/debian/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg && \
    echo \
      "deb http://deb.debian.org/debian/ experimental main" | tee -a /etc/apt/sources.list.d/docker.list > /dev/null && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \  
    add-apt-repository -y "deb http://deb.debian.org/debian/ testing main" && \
    apt-get update && \
    apt-get install --no-install-recommends -y libstdc++6 libc6 libnsl2 nano && \
    rm -rf /var/lib/apt/lists/*

ARG ZOMBIENET_RELEASE
RUN wget https://github.com/paritytech/zombienet/releases/download/$ZOMBIENET_RELEASE/zombienet-linux-x64 && \
    chmod +x zombienet-linux-x64 && \
    cp zombienet-linux-x64 /home/nonroot/bin && \
    rm -rf zombienet-linux-x64

RUN chown -R root. /home/nonroot