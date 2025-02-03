# Shared Environment Variables
# bookworm (Debian 12)
MINIDEB_IMAGE=bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16
OS_IMAGE_NAME ?= ubuntu
OS_IMAGE_TAG ?= 24.04
OS_IMAGE_HASH ?= 99c35190e22d294cdace2783ac55effc69d32896daaa265f0bbedbcde4fbe3e5
## linux/amd64, linux/arm64
PLATFORM ?= linux/amd64
ARCHITECTURE ?= $(subst linux/,,$(PLATFORM))
## DOCKER_REGISTRY_PATH empty for local usage
DOCKER_REGISTRY_PATH ?=
DOCKER_BUILD_DIR_NAME ?= build_docker_$(ARCHITECTURE)
GOOGLE_APPLICATION_CREDENTIALS ?=

# Generated Variables
OS_IMAGE ?= $(OS_IMAGE_NAME):$(OS_IMAGE_TAG)@sha256:$(OS_IMAGE_HASH)
OS_IMAGE_TAG_WITH_HASH := $(OS_IMAGE_TAG)@sha256:$(OS_IMAGE_HASH)
OS_IMAGE_SHORT_HASH := $(shell echo $(OS_IMAGE_HASH) | cut -c1-7)
USER_ID = 5555
GROUP_ID = 5555

# polkadot_builder Variables
POLKADOT_SDK_TAG ?=
POLKADOT_SDK_RELEASE ?=
RUST_VERSION ?= 1.81.0
SCCACHE_VERSION ?= 0.9.0
BUILDER_LATEST_TAG ?= latest
TESTER_LATEST_TAG ?= latest
IN_DOCKER_USERNAME := builder
IN_DOCKER_WORKING_DIR := /home/$(IN_DOCKER_USERNAME)

# polkadot_binary Variables
SCCACHE_GCS_BUCKET ?=
SCCACHE_GCS_KEY_PREFIX ?= polkadot_builder_$(ARCHITECTURE)
POLKADOT_REPO_URL ?= https://github.com/paritytech/polkadot-sdk.git
POLKADOT_REPO_DIR ?= ./polkadot-sdk
POLKADOT_BUILD_CONTAINER_NAME := polkadot_build 

RESULT_BIN_NAMES=polkadot polkadot-parachain malus undying-collator adder-collator polkadot-execute-worker polkadot-prepare-worker
BUILD_COMMANDS = \
	time cargo build --locked --profile release --features pyroscope,fast-runtime \
		--bin polkadot --bin polkadot-prepare-worker --bin polkadot-execute-worker && \
	time cargo build --locked --profile release \
		-p polkadot-parachain-bin -p polkadot-test-malus -p test-parachain-adder-collator -p test-parachain-undying-collator \
		--bin polkadot-parachain --bin malus --bin undying-collator --bin adder-collator
RESULT_BINARIES_WITH_PATH := $(patsubst %,./target/release/%,$(RESULT_BIN_NAMES))
POLKADOT_BINARY_DEPENDENCIES=libstdc++6, zlib1g, libgcc-s1, libc6, ca-certificates

# upload_apt_package Variables
ARTIFACTS_REPO ?= kagome-apt
REGION ?= europe-north1

# zombie_tester Variables
PROJECT_ID ?=
POLKADOT_BINARY_PACKAGE_VERSION ?=

# copy_logs_to_host Variables
# COPY_LOGS_TO_HOST: boolean flag to determine whether to copy logs to host
COPY_LOGS_TO_HOST ?= true
HOST_LOGS_PATH ?= /tmp/test_logs
CONTAINER_NAME ?= zombienet-test

# tests Variables
ZOMBIE_TESTER_IMAGE_TAG ?= latest
ZOMBIE_TESTER_IMAGE ?= zombie_tester:$(ZOMBIE_TESTER_IMAGE_TAG)
KAGOME_PACKAGE_VERSION ?=
WORKING_DIR := $(shell pwd)/../../../kagome
DELETE_IMAGE_AFTER_TEST ?= true

CURRENT_DATE := $(shell TZ=UTC-3 date +"%Y%m%d")

export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto
