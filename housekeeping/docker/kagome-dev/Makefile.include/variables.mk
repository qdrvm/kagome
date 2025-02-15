# Shared Environment Variables
OS_IMAGE_NAME ?= ubuntu
OS_IMAGE_TAG ?= 24.04
OS_IMAGE_TAG_CLEAN ?= $(subst .,,$(OS_IMAGE_TAG))
OS_IMAGE_HASH ?= 72297848456d5d37d1262630108ab308d3e9ec7ed1c3286a32fe09856619a782
OS_REL_VERSION ?= noble

# kagome_builder Variables
RUST_VERSION ?= 1.81.0
GCC_VERSION ?= 13
LLVM_VERSION ?= 19

## DOCKER_REGISTRY_PATH empty for local usage
DOCKER_REGISTRY_PATH ?=
DOCKERHUB_REGISTRY_PATH ?= qdrvm/kagome
GOOGLE_APPLICATION_CREDENTIALS ?=
PLATFORM ?= linux/amd64
ARCHITECTURE ?= $(subst linux/,,$(PLATFORM))

# Debug, Release, RelWithDebInfo
BUILD_TYPE ?= Release

# kagome_dev_docker_build Variables
BUILD_DIR ?= build
CACHE_DIR := $(shell pwd)/../../../../kagome/$(BUILD_DIR)/cache
WORKING_DIR := $(shell pwd)/../../../../kagome
DEPENDENCIES ?= libstdc++6, zlib1g, libgcc-s1, libc6, libtinfo6, libseccomp2, libatomic1, libnsl2
GITHUB_HUNTER_USERNAME ?=
GITHUB_HUNTER_TOKEN ?=
CTEST_OUTPUT_ON_FAILURE ?= 1
WERROR ?= OFF
SAN_PARAMS ?= -DCLEAR_OBJS=ON -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-13_cxx20.cmake -DASAN=ON
UBSAN_OPTIONS ?= print_stacktrace=1

# Generated versions
OS_IMAGE ?= $(OS_IMAGE_NAME):$(OS_IMAGE_TAG)@sha256:$(OS_IMAGE_HASH)
OS_IMAGE_TAG_WITH_HASH := $(OS_IMAGE_TAG)@sha256:$(OS_IMAGE_HASH)
OS_IMAGE_SHORT_HASH := $(shell echo $(OS_IMAGE_HASH) | cut -c1-7)
BUILDER_IMAGE_TAG ?= ub$(OS_IMAGE_TAG_CLEAN)-$(OS_IMAGE_SHORT_HASH)_rust$(RUST_VERSION)_gcc$(GCC_VERSION)_llvm$(LLVM_VERSION)
BUILDER_LATEST_TAG ?= latest
TESTER_LATEST_TAG ?= latest
DOCKERHUB_BUILDER_PATH ?= $(DOCKERHUB_REGISTRY_PATH)_builder
KAGOME_SANITIZED_VERSION = $(shell cd $(WORKING_DIR) && ./get_version.sh --sanitized)

# kagome_runtime_cache and kagome_image_build Variables
KAGOME_PACKAGE_VERSION ?=
#KAGOME_RUNTIME_PACKAGE_VERSION ?=

# upload_apt_package Variables
ARTIFACTS_REPO ?= kagome-apt
PUBLIC_ARTIFACTS_REPO ?= kagome
REGION ?= europe-north1

# CI Variables
IS_MAIN_OR_TAG ?= false
GIT_REF_NAME ?=
CI ?= false
BUILD_THREADS=$(shell nproc 2>/dev/null || sysctl -n hw.ncpu)

export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto
