# Debug, Release, RelWithDebInfo
BUILD_TYPE ?= Release

BUILD_DIR ?= build
BUILD_FINAL_TARGET ?= kagome

# Build options
BACKWARD ?= OFF
WERROR ?= OFF

# Macos build options
CMAKE_VERSION ?= 3.25
RUST_VERSION ?= 1.79.0

# CI Options
CI ?= false
GITHUB_RUNNER ?= false

build:
	mkdir -p $(BUILD_DIR) && \
	echo \"Building in $$(pwd)\" && \
	BUILD_THREADS=$(nproc 2>/dev/null || sysctl -n hw.ncpu); \
	cmake . -B"$(BUILD_DIR)" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBACKWARD=$(BACKWARD) -DWERROR=$(WERROR) && \
	cmake --build "$(BUILD_DIR)" --target $(BUILD_FINAL_TARGET) -- -j${BUILD_THREADS}

macos_install_deps:
	if [[ "$(CI)" = "true" ]]; then \
		python3 -m venv ~/venv ; \
		source ~/venv/bin/activate ; \
    fi ; \
    sudo python3 -m pip install --upgrade pip ; \
    sudo python3 -m pip install cmake==$(CMAKE_VERSION) scikit-build requests gitpython gcovr ; \
    curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain $(RUST_VERSION) --profile minimal ; \
    brew install ninja

macos_build:
	mkdir -p $(BUILD_DIR) && \
	echo \"Building in $$(pwd)\" && \
	BUILD_THREADS=$$(nproc 2>/dev/null || sysctl -n hw.ncpu); \
	if [ "$(CI)" = "true" ]; then \
		python3 -m venv ~/venv ; \
		source ~/venv/bin/activate ; \
		export HUNTER_PYTHON_LOCATION=~/venv/bin/python3 ; \
    fi ; \
	if [ "$(GITHUB_RUNNER)" = "true" ]; then \
		git config --global --add safe.directory /__w/kagome/kagome ; \
	fi ; \
	export SDKROOT=$$(xcrun --sdk macosx --show-sdk-path) ; \
	export CURL_SSL_BACKEND=SecureTransport ; \
	cmake . -B"$(BUILD_DIR)" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBACKWARD=$(BACKWARD) -DWERROR=$(WERROR) && \
	cmake --build "$(BUILD_DIR)" --target $(BUILD_FINAL_TARGET) -- -j${BUILD_THREADS}

docker: 
	INDOCKER_IMAGE=qdrvm/kagome-dev:minideb BUILD_DIR=build BUILD_THREADS=$(shell expr $(shell nproc 2>/dev/null || sysctl -n hw.ncpu) + 1 ) ./housekeeping/indocker.sh ./housekeeping/make_build.sh


command:
	INDOCKER_IMAGE=qdrvm/kagome-dev:minideb ./housekeeping/indocker.sh ${args}


release:
	INDOCKER_IMAGE=qdrvm/kagome-dev:minideb BUILD_DIR=build ./housekeeping/indocker.sh ./housekeeping/docker/release/makeRelease.sh


release_docker:
	VERSION=0.0.1 BUILD_DIR=build BUILD_TYPE=Release ./housekeeping/docker/kagome/build_and_push.sh


debug_docker:
	VERSION=0.0.1 BUILD_DIR=build BUILD_TYPE=Debug ./housekeeping/docker/kagome/build_and_push.sh

custom_docker:
	VERSION=0.0.1 BUILD_TYPE=Custom ./housekeeping/docker/kagome/build_and_push.sh

clear:
	rm -rf build
	rm -rf housekeeping/docker/kagome-dev/pkg
