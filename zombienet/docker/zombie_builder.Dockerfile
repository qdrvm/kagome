ARG POLKADOT_RELEASE_GLOBAL
ARG POLKADOT_RELEASE_GLOBAL_NUMERIC
ARG POLKADOT_SDK_RELEASE
ARG ZOMBIENET_RELEASE

ARG BASE_IMAGE=bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16

# bookworm
FROM ${BASE_IMAGE} as polkadot-sdk-builder

USER root

ARG POLKADOT_SDK_RELEASE
ENV POLKADOT_SDK_RELEASE=$POLKADOT_SDK_RELEASE

WORKDIR /home/nonroot/

RUN install_packages  \
        bash \
        git  \
        ssh  \
        curl  \
        nano  \
        build-essential  \
        protobuf-compiler  \
        libprotobuf-dev  \
        time  \
        clang  \
        ca-certificates  \
        openssl

SHELL ["/bin/bash", "-c"]

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y

ENV RUSTUP_HOME="/opt/rust"
ENV CARGO_HOME="/opt/cargo"
ENV PATH="/root/.cargo/bin:${PATH}"

RUN rustup default stable &&  \
    rustup target add wasm32-unknown-unknown &&  \
    rustup component add rust-src

RUN groupadd --gid 10001 nonroot && \
    useradd  --home-dir /home/nonroot \
             --create-home \
             --shell /bin/bash \
             --gid nonroot \
             --groups nonroot \
             --uid 10000 nonroot


FROM polkadot-sdk-builder AS polkadot-sdk-bin

ENV RUSTUP_HOME="/opt/rust"
ENV CARGO_HOME="/opt/cargo"
ENV PATH="/root/.cargo/bin:${PATH}"

RUN git clone --depth 1 --branch $POLKADOT_SDK_RELEASE https://github.com/paritytech/polkadot-sdk.git

WORKDIR /home/nonroot/polkadot-sdk/

RUN cargo update  \
      -p test-parachain-adder-collator \
      -p polkadot-test-malus \
      -p test-parachain-undying-collator

RUN cargo build --profile testnet  \
      -p test-parachain-adder-collator \
      -p polkadot-test-malus \
      -p test-parachain-undying-collator

RUN find /home/nonroot/polkadot-sdk/target/ -maxdepth 2 -print


FROM ${BASE_IMAGE} as zombienet-bin

ARG ZOMBIENET_RELEASE
ENV ZOMBIENET_RELEASE=$ZOMBIENET_RELEASE

WORKDIR /home/nonroot/

RUN install_packages  \
        bash \
        wget  \
        nano \
        ca-certificates

SHELL ["/bin/bash", "-c"]

RUN mkdir -p /home/nonroot/bin
    
WORKDIR /home/nonroot/bin

ENV PATH="/home/nonroot/bin:${PATH}"

RUN wget https://github.com/paritytech/zombienet/releases/download/$ZOMBIENET_RELEASE/zombienet-linux-x64 && \
    chmod +x zombienet-linux-x64 && \
    ln -s /home/nonroot/bin/zombienet-linux-x64 /home/nonroot/bin/zombienet

# WARNING: Setup always returns 2, even when successful 
RUN zombienet setup -y polkadot polkadot-parachain; \
    EXIT_CODE=$?; \
    if [ $EXIT_CODE -eq 2 ]; then \
        echo "Command exited with code 2, continuing..."; \
    else \
        echo "Command exited with code $EXIT_CODE"; \
        exit $EXIT_CODE; \
    fi;

COPY --from=polkadot-sdk-bin /home/nonroot/polkadot-sdk/target/testnet/malus /home/nonroot/bin
COPY --from=polkadot-sdk-bin /home/nonroot/polkadot-sdk/target/testnet/adder-collator /home/nonroot/bin
COPY --from=polkadot-sdk-bin /home/nonroot/polkadot-sdk/target/testnet/undying-collator /home/nonroot/bin
#COPY polkadot-sdk-versions.txt /home/nonroot/polkadot-sdk-versions.txt
#COPY zombienet-versions.txt /home/nonroot/zombienet-versions.txt

RUN ./polkadot --version && \
    ./polkadot-parachain --version && \
    ./zombienet version  && \
    ./polkadot-execute-worker --version && \
    ./polkadot-prepare-worker --version && \
    ./malus --version && \
    ./adder-collator --version && \
    ./undying-collator --version

FROM zombienet-bin as final

ENV PATH="/home/nonroot/bin:${PATH}"
ARG ZOMBIENET_RELEASE
ENV ZOMBIENET_RELEASE=$ZOMBIENET_RELEASE
ARG POLKADOT_SDK_RELEASE
ENV POLKADOT_SDK_RELEASE=$POLKADOT_SDK_RELEASE

RUN install_packages  \
      curl  \
      gpg  \
      gpg-agent  \
      software-properties-common \
      libstdc++6  \
      libc6  \
      libnsl2

