# CI Variables
IS_MAIN_OR_TAG ?= false
GIT_REF_NAME ?= 
CI ?= false
BUILD_THREADS=$(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
CURRENT_DATE := $(shell TZ=UTC-3 date +"%Y%m%d")

# Shared Environment Variables
OS_IMAGE_NAME ?= ubuntu
OS_IMAGE_TAG ?= 24.04
OS_IMAGE_TAG_CLEAN ?= $(subst .,,$(OS_IMAGE_TAG))
OS_IMAGE_HASH ?= 72297848456d5d37d1262630108ab308d3e9ec7ed1c3286a32fe09856619a782
OS_REL_VERSION ?= noble
OS_IMAGE_SHORT_HASH := $(shell echo $(OS_IMAGE_HASH) | cut -c1-7)
USER_ID ?= $(shell id -u)
GROUP_ID ?= $(shell id -g)
DIR_PATH_HASH := $(shell echo -n $(CURDIR) | sha1sum | cut -c1-8 )
HOST_OS := $(shell uname -s)
# For Docker build
# USER_ID = 5555
# GROUP_ID = 5555

# kagome_builder Variables
RUST_VERSION ?= 1.81.0
GCC_VERSION ?= 13
LLVM_VERSION ?= 19
IN_DOCKER_USERNAME := builder
IN_DOCKER_HOME := /home/$(IN_DOCKER_USERNAME)

## DOCKER_REGISTRY_PATH empty for local usage
DOCKER_REGISTRY_PATH ?=
DOCKERHUB_REGISTRY_PATH ?= qdrvm/kagome
GOOGLE_APPLICATION_CREDENTIALS ?=
PLATFORM ?= linux/amd64
ARCHITECTURE ?= $(subst linux/,,$(PLATFORM))

# Debug, Release, RelWithDebInfo
BUILD_TYPE ?= Release

# kagome_docker_build Variables
CONTAINER_NAME := kagome_build-$(DIR_PATH_HASH)
DOCKER_BUILD_DIR_NAME := build_docker_$(ARCHITECTURE)
WORKING_DIR := $(shell pwd)/../../../../kagome
BUILD_DIR := $(WORKING_DIR)/$(DOCKER_BUILD_DIR_NAME)
CACHE_DIR := $(BUILD_DIR)/cache
DEPENDENCIES ?= libstdc++6, zlib1g, libgcc-s1, libc6, libtinfo6, libseccomp2, libatomic1, libnsl2
GITHUB_HUNTER_USERNAME ?=
GITHUB_HUNTER_TOKEN ?=
CTEST_OUTPUT_ON_FAILURE ?= 1
WERROR ?= OFF
SAN_PARAMS ?= -DCLEAR_OBJS=ON -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-13_cxx20.cmake -DASAN=ON
UBSAN_OPTIONS ?= print_stacktrace=1
# Hunter debug flags -DHUNTER_STATUS_DEBUG=ON -DHUNTER_USE_CACHE_SERVERS=NO
BUILD_COMMANDS = \
	time cmake . -B\"$(DOCKER_BUILD_DIR_NAME)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" \
		-DBACKWARD=OFF -DWERROR=$(WERROR) && \
	time cmake --build \"$(DOCKER_BUILD_DIR_NAME)\" --target kagome -- -j$(BUILD_THREADS)

# Generated versions
OS_IMAGE ?= $(OS_IMAGE_NAME):$(OS_IMAGE_TAG)@sha256:$(OS_IMAGE_HASH)
OS_IMAGE_TAG_WITH_HASH := $(OS_IMAGE_TAG)@sha256:$(OS_IMAGE_HASH)
OS_IMAGE_SHORT_HASH := $(shell echo $(OS_IMAGE_HASH) | cut -c1-7)
BUILDER_IMAGE_TAG ?= ub$(OS_IMAGE_TAG_CLEAN)-$(OS_IMAGE_SHORT_HASH)_rust$(RUST_VERSION)_gcc$(GCC_VERSION)_llvm$(LLVM_VERSION)
BUILDER_LATEST_TAG ?= latest
TESTER_LATEST_TAG ?= latest
DOCKERHUB_BUILDER_PATH ?= $(DOCKERHUB_REGISTRY_PATH)_builder
KAGOME_SANITIZED_VERSION = $(shell cd $(WORKING_DIR) && ./get_version.sh --sanitized)
SHORT_COMMIT_HASH=$(shell grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2)

# kagome_runtime_cache and kagome_image_build Variables
KAGOME_PACKAGE_VERSION ?=
#KAGOME_RUNTIME_PACKAGE_VERSION ?=
RUNTIME_VERSION := $(shell echo "`date +'%y.%m.%d'`-`python3 get_wasmedge_version.py`")

# upload_apt_package Variables
ARTIFACTS_REPO ?= kagome-apt
PUBLIC_ARTIFACTS_REPO ?= kagome
REGION ?= europe-north1

# cache management Variables
CACHE_BUCKET ?= ci-cache-kagome
ZSTD_LEVEL ?= 5
ZSTD_THREADS = $(shell nproc 2>/dev/null || sysctl -n hw.ncpu)
CACHE_TAG = $(CURRENT_DATE)-$(HOST_OS)-$(ARCHITECTURE)-$(BUILD_TYPE)
CACHE_ARCHIVE = build-cache-$(CACHE_TAG).tar.zst
CACHE_UPLOAD_ALLOWED ?= false
CACHE_LIFETIME_DAYS ?= 3
CACHE_ONLY_MASTER ?= false


export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto
