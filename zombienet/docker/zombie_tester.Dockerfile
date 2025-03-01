ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE
ARG BASE_IMAGE_TAG

ARG PROJECT_ID
ARG POLKADOT_DEB_PACKAGE_VERSION_NO_ARCH
ARG ZOMBIENET_RELEASE

ARG REGION=europe-north1
ARG ARCHITECTURE


FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG} AS zombie-tester

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Zombienet image"

# Use a temporary working directory for root operations
WORKDIR /root/
SHELL ["/bin/bash", "-xo", "pipefail", "-c"]

# Copy the install_packages script and set executable permissions
COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

# Set Polkadot SDK release environment variable and install required packages
ARG ZOMBIENET_RELEASE
ENV ZOMBIENET_RELEASE=$ZOMBIENET_RELEASE

# Setup enterprise repository and install required packages
ARG REGION
ENV REGION=$REGION
ARG PROJECT_ID
ENV PROJECT_ID=$PROJECT_ID
ARG POLKADOT_DEB_PACKAGE_VERSION
ENV POLKADOT_DEB_PACKAGE_VERSION=$POLKADOT_DEB_PACKAGE_VERSION

RUN install_packages  \
        wget  \
        nano \
        ca-certificates \
        gnupg2 \
        curl \
        nodejs \
        npm

# Install Zombienet
# RUN npm install -g @zombienet/cli@${ZOMBIENET_RELEASE}

RUN ARCHITECTURE=$(uname -m); \
    if [ "$ARCHITECTURE" = "x86_64" ]; then \
        ARCH="x64"; \
    elif [ "$ARCHITECTURE" = "aarch64" ]; then \
        ARCH="arm64"; \
    else \
        echo "-- Unsupported architecture: $ARCHITECTURE"; exit 1; \
    fi && \
    echo "-- Downloading zombienet v${ZOMBIENET_RELEASE} for ${ARCH}" && \
    wget -q "https://github.com/paritytech/zombienet/releases/download/v${ZOMBIENET_RELEASE}/zombienet-linux-${ARCH}" -O /usr/local/bin/zombienet && \
    chmod +x /usr/local/bin/zombienet && \
    ln -s /usr/local/bin/zombienet /usr/bin/zombienet

# Install Polkadot SDK
RUN curl -fsSL https://${REGION}-apt.pkg.dev/doc/repo-signing-key.gpg | \
      gpg --dearmor -o /usr/share/keyrings/${REGION}-apt-archive-keyring.gpg

RUN curl -fsSL https://packages.cloud.google.com/apt/doc/apt-key.gpg | \
      gpg --dearmor -o /usr/share/keyrings/cloud-google-apt-archive-keyring.gpg
    
RUN echo "deb [signed-by=/usr/share/keyrings/${REGION}-apt-archive-keyring.gpg] \
      http://packages.cloud.google.com/apt apt-transport-artifact-registry-stable main" | \
      tee -a /etc/apt/sources.list.d/artifact-registry.list

RUN install_packages apt-transport-artifact-registry

RUN echo "deb [signed-by=/usr/share/keyrings/europe-north1-apt-archive-keyring.gpg] \
      ar+https://${REGION}-apt.pkg.dev/projects/${PROJECT_ID} kagome-apt main" | \
      tee -a  /etc/apt/sources.list.d/kagome.list

RUN sed -i 's|^\(\s*\)# *Service-Account-JSON ".*";|\1Service-Account-JSON "/root/.gcp/google_creds.json";|' \
      /etc/apt/apt.conf.d/90artifact-registry

RUN mkdir -p /root/.gcp

RUN --mount=type=secret,id=google_creds,target=/root/.gcp/google_creds.json \
      install_packages polkadot-binary="${POLKADOT_DEB_PACKAGE_VERSION}" && \
      sed -i '1s/^/#/' /etc/apt/sources.list.d/kagome.list

# Check installed versions
RUN echo "Polkadot Version:" && polkadot --version && \
    echo "Polkadot Parachain Version:" && polkadot-parachain --version && \
    echo "Polkadot Execute Worker Version:" && polkadot-execute-worker --version && \
    echo "Polkadot Prepare Worker Version:" && polkadot-prepare-worker --version && \
    echo "Malus Version:" && malus --version && \
    echo "Adder Collator Version:" && adder-collator --version && \
    echo "Undying Collator Version:" && undying-collator --version  && \
    echo "Zombienet Version:" && zombienet version

# Install required packages
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

# Create non-root user and switch to it
ARG USER_ID=1000
ARG GROUP_ID=1000
ARG USER_NAME=builder
ENV USER_ID=${USER_ID}
ENV GROUP_ID=${GROUP_ID}
ENV USER_NAME=${USER_NAME}

# Create group with the specified GID, and user with the specified UID. Switch to the user.
RUN groupadd -g ${GROUP_ID} ${USER_NAME} && \
    useradd -m -d /home/${USER_NAME} -u ${USER_ID} -g ${USER_NAME} ${USER_NAME}

WORKDIR /home/${USER_NAME}/workspace

# Install kagome on the startup
ARG KAGOME_PACKAGE_VERSION
ENV KAGOME_PACKAGE_VERSION=${KAGOME_PACKAGE_VERSION}

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod 0755 /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

HEALTHCHECK --interval=5s --timeout=2s --start-period=0s --retries=100 \
  CMD test -f /tmp/entrypoint_done || exit 1
