ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG
ARG RUST_VERSION

ARG PROJECT_ID
ARG POLKADOT_BINARY_PACKAGE_VERSION
ARG POLKADOT_SDK_RELEASE
ARG ZOMBIENET_RELEASE

ARG REGION=europe-north1
ARG ARCHITECTURE=x86_64


FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG} AS zombie-tester

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Zombienet image"

ARG ZOMBIENET_RELEASE
ENV ZOMBIENET_RELEASE=$ZOMBIENET_RELEASE
ARG POLKADOT_SDK_RELEASE
ENV POLKADOT_SDK_RELEASE=$POLKADOT_SDK_RELEASE

COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN groupadd --gid 10000 nonroot && \
    useradd --home-dir /home/nonroot \
            --create-home \
            --shell /bin/bash \
            --gid nonroot \
            --groups nonroot \
            --uid 10000 nonroot

WORKDIR /home/nonroot/

RUN install_packages  \
        wget  \
        nano \
        ca-certificates \
        gnupg2 \
        curl


RUN mkdir -p /home/nonroot/bin
    
WORKDIR /home/nonroot/bin

ENV PATH="/home/nonroot/bin:${PATH}"

RUN wget -q https://github.com/paritytech/zombienet/releases/download/$ZOMBIENET_RELEASE/zombienet-linux-x64 && \
    chmod +x zombienet-linux-x64 && \
    ln -s /home/nonroot/bin/zombienet-linux-x64 /home/nonroot/bin/zombienet

# Setup enterprise repository

ARG REGION
ENV REGION=$REGION

RUN curl -fsSL https://${REGION}-apt.pkg.dev/doc/repo-signing-key.gpg | \
      gpg --dearmor -o /usr/share/keyrings/${REGION}-apt-archive-keyring.gpg

RUN curl -fsSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | \
      gpg --dearmor -o /usr/share/keyrings/cloud-google-apt-archive-keyring.gpg
    
RUN echo "deb [signed-by=/usr/share/keyrings/${REGION}-apt-archive-keyring.gpg] \
      http://packages.cloud.google.com/apt apt-transport-artifact-registry-stable main" | \
      tee -a /etc/apt/sources.list.d/artifact-registry.list

RUN install_packages apt-transport-artifact-registry

ARG PROJECT_ID
ENV PROJECT_ID=$PROJECT_ID

RUN echo "deb [signed-by=/usr/share/keyrings/europe-north1-apt-archive-keyring.gpg] \
      ar+https://${REGION}-apt.pkg.dev/projects/${PROJECT_ID} kagome-apt main" | \
      tee -a  /etc/apt/sources.list.d/kagome.list

RUN sed -i 's|^\(\s*\)# *Service-Account-JSON ".*";|\1Service-Account-JSON "/root/.gcp/google_creds.json";|' \
      /etc/apt/apt.conf.d/90artifact-registry

ARG POLKADOT_BINARY_PACKAGE_VERSION
ENV POLKADOT_BINARY_PACKAGE_VERSION=$POLKADOT_BINARY_PACKAGE_VERSION

RUN mkdir -p /root/.gcp
RUN --mount=type=secret,id=google_creds,target=/root/.gcp/google_creds.json \
      install_packages polkadot-binary=${POLKADOT_BINARY_PACKAGE_VERSION} && \
      sed -i '1s/^/#/' /etc/apt/sources.list.d/kagome.list

RUN echo "Polkadot Version:" && ./polkadot --version && \
    echo "Polkadot Parachain Version:" && ./polkadot-parachain --version && \
    echo "Zombienet Version:" && ./zombienet version && \
    echo "Polkadot Execute Worker Version:" && ./polkadot-execute-worker --version && \
    echo "Polkadot Prepare Worker Version:" && ./polkadot-prepare-worker --version && \
    echo "Malus Version:" && malus --version && \
    echo "Adder Collator Version:" && adder-collator --version && \
    echo "Undying Collator Version:" && undying-collator --version

RUN ln -s /home/nonroot/bin/zombienet-linux-x64 /usr/local/bin/zombienet && \
    ln -s /home/nonroot/bin/polkadot /usr/local/bin/polkadot && \
    ln -s /home/nonroot/bin/polkadot-parachain /usr/local/bin/polkadot-parachain && \
    ln -s /home/nonroot/bin/polkadot-execute-worker /usr/local/bin/polkadot-execute-worker && \
    ln -s /home/nonroot/bin/polkadot-prepare-worker /usr/local/bin/polkadot-prepare-worker

RUN install_packages  \
      gpg  \
      gpg-agent  \
      git \
      time \
      software-properties-common \
      libstdc++6  \
      libc6  \
      libnsl2 \
      libtinfo6 \
      libseccomp2 \
      libatomic1 \
      ssh \
      libgcc-s1 \
      zlib1g
