ARG AUTHOR="k.azovtsev@qdrvm.io <Kirill Azovtsev>"

ARG BASE_IMAGE

ARG PROJECT_ID

ARG REGION=europe-north1
ARG ARCHITECTURE=x86_64

ARG KAGOME_PACKAGE_VERSION

FROM ${BASE_IMAGE} AS base

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Kagome image"

COPY install_packages /usr/sbin/install_packages
RUN chmod 0755 /usr/sbin/install_packages

RUN install_packages \
        bash \
        software-properties-common \
        curl \
        wget \
        nano \
        gpg \
        gpg-agent \
        tini

SHELL ["/bin/bash", "-c"]

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

RUN mkdir -p /root/.gcp

ARG KAGOME_PACKAGE_VERSION
ENV KAGOME_PACKAGE_VERSION=${KAGOME_PACKAGE_VERSION}

RUN --mount=type=secret,id=google_creds,target=/root/.gcp/google_creds.json \
      install_packages \
        kagome-dev=${KAGOME_PACKAGE_VERSION} && \
        sed -i '1s/^/#/' /etc/apt/sources.list.d/kagome.list

CMD ["/usr/bin/tini", "--", "/bin/bash", "-c"]


FROM base AS debug

ARG AUTHOR
ENV AUTHOR=${AUTHOR}
LABEL org.opencontainers.image.authors="${AUTHOR}"
LABEL org.opencontainers.image.description="Kagome debug image"

RUN install_packages \
        gdb \
        gdbserver
