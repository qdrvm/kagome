# Build environment for sanitizer runners
ARG BASE_IMAGE=ubuntu
ARG BASE_IMAGE_TAG=24.04

FROM ${BASE_IMAGE}:${BASE_IMAGE_TAG} AS base

# Install package installation helper
COPY ./install_packages.sh /usr/bin/install_packages
RUN chmod +x /usr/bin/install_packages

# Set up Kagome repositories
RUN --mount=type=secret,id=google_creds,target=/root/.gcp/google_creds.json \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-transport-https \
        ca-certificates \
        curl \
        gnupg \
        lsb-release && \
    export CLOUD_SDK_REPO="cloud-sdk-$(lsb_release -c -s)" && \
    echo "deb [signed-by=/usr/share/keyrings/cloud.google.gpg] https://packages.cloud.google.com/apt cloud-sdk main" | tee -a /etc/apt/sources.list.d/google-cloud-sdk.list && \
    curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | apt-key --keyring /usr/share/keyrings/cloud.google.gpg add - && \
    apt-get update && \
    apt-get install -y google-cloud-cli && \
    echo "Activating service account..." && \
    gcloud auth activate-service-account --key-file=/root/.gcp/google_creds.json && \
    PROJECT_ID=$(cat /root/.gcp/google_creds.json | grep "project_id" | cut -d'"' -f4) && \
    echo "Configuring apt repository in ${PROJECT_ID}..." && \
    curl -s https://storage.googleapis.com/artifacts.${PROJECT_ID}.appspot.com/apt/doc/repo-key.gpg | apt-key add - && \
    echo "deb https://artifacts-apt.europe-north1.packages.${PROJECT_ID}.appspot.com kagome-apt main" | tee /etc/apt/sources.list.d/kagome.list && \
    apt-get update

FROM base as runner
ARG PACKAGE_NAME=kagome-asan
ARG KAGOME_PACKAGE_VERSION
ARG PROJECT_ID
ARG ARCHITECTURE=amd64

# Set environment variables for sanitizers
ENV ASAN_OPTIONS=symbolize=1:abort_on_error=1
ENV LSAN_OPTIONS=suppressions=/etc/lsan.supp:print_suppressions=0
ENV TSAN_OPTIONS=suppressions=/etc/tsan.supp:halt_on_error=1:history_size=7
ENV UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

COPY ./sanitizer_suppressions/lsan.supp /etc/lsan.supp
COPY ./sanitizer_suppressions/tsan.supp /etc/tsan.supp

# Install sanitizer-specific package
RUN --mount=type=secret,id=google_creds,target=/root/.gcp/google_creds.json \
      install_packages \
        ${PACKAGE_NAME}=${KAGOME_PACKAGE_VERSION} && \
        sed -i '1s/^/#/' /etc/apt/sources.list.d/kagome.list

WORKDIR /kagome
ENV LD_LIBRARY_PATH=/tmp/kagome/runtimes-cache/
ENTRYPOINT ["kagome"]
