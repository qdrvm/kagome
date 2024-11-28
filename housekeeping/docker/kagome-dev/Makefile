# Shared Environment Variables
OS_IMAGE_NAME ?= ubuntu
OS_IMAGE_TAG ?= 24.04
OS_IMAGE_HASH ?= 99c35190e22d294cdace2783ac55effc69d32896daaa265f0bbedbcde4fbe3e5

# kagome_builder Variables
RUST_VERSION ?= 1.81.0
GCC_VERSION ?= 13
LLVM_VERSION ?= 19

## DOCKER_REGISTRY_PATH empty for local usage
DOCKER_REGISTRY_PATH ?=
DOCKERHUB_REGISTRY_PATH ?= qdrvm/kagome
GOOGLE_APPLICATION_CREDENTIALS ?=
PLATFORM ?= linux/amd64
ARCHITECTURE ?= x86_64
PACKAGE_ARCHITECTURE ?= amd64

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
BUILDER_IMAGE_TAG ?= $(OS_IMAGE_SHORT_HASH)_rust$(RUST_VERSION)_gcc$(GCC_VERSION)_llvm$(LLVM_VERSION)
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

export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto

get_versions:
	@echo "full_commit_hash: `git rev-parse HEAD`" | tee commit_hash.txt
	@echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" | tee -a commit_hash.txt
	@echo "kagome_version: `cd $(WORKING_DIR) && ./get_version.sh`" | tee kagome_version.txt
	@echo "kagome_sanitized_version: $(KAGOME_SANITIZED_VERSION)" | tee -a kagome_version.txt

builder_image_tag:
	@echo $(BUILDER_IMAGE_TAG)

kagome_builder:
	docker build --platform $(PLATFORM) \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG) \
    		-f kagome_builder.Dockerfile \
    		--build-arg RUST_VERSION=$(RUST_VERSION) \
    		--build-arg GCC_VERSION=$(GCC_VERSION) \
    		--build-arg LLVM_VERSION=$(LLVM_VERSION) \
    		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
    		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
    		--build-arg ARCHITECTURE=$(ARCHITECTURE) .

kagome_builder_push:
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) ; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG)

kagome_builder_push_dockerhub:
	docker tag $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) ; \
	docker tag $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG) $(DOCKERHUB_BUILDER_PATH):$(BUILDER_LATEST_TAG) ; \
	docker push $(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) ; \
	docker push $(DOCKERHUB_BUILDER_PATH):$(BUILDER_LATEST_TAG)

kagome_dev_docker_build:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	CONTAINER_NAME=kagome_dev_build_$$(openssl rand -hex 6); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_THREADS=$$(nproc 2>/dev/null || sysctl -n hw.ncpu); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Build type: $(BUILD_TYPE)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e BUILD_THREADS=$$BUILD_THREADS \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $$(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"$(BUILD_DIR)\" --target kagome -- -j${BUILD_THREADS} && \
		mkdir -p /tmp/kagome && \
		cp /opt/kagome/$(BUILD_DIR)/node/kagome /tmp/kagome/kagome && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		./build_apt_package.sh \
			\"$$(date +'%y.%m.%d')-$${SHORT_COMMIT_HASH}-$(BUILD_TYPE)\" \
			$(PACKAGE_ARCHITECTURE) \
			kagome-dev \
			/tmp/kagome \
			'Kagome Dev Ubuntu Package' \
			'$(DEPENDENCIES)' && \
		if [ "$(IS_MAIN_OR_TAG)" = "true" ] && [ "$(GIT_REF_NAME)" != "master" ] && [ "$(BUILD_TYPE)" = "Release" ]; then \
			./build_apt_package.sh \
				\"$(KAGOME_SANITIZED_VERSION)-$(BUILD_TYPE)\" \
				$(PACKAGE_ARCHITECTURE) \
				kagome \
				/tmp/kagome \
				'Kagome Ubuntu Package' \
				'$(DEPENDENCIES)' ; \
		fi; \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME

kagome_dev_docker_build_tidy_inhouse:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	CONTAINER_NAME=kagome_dev_build_$$(openssl rand -hex 6); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_THREADS=$$(nproc 2>/dev/null || sysctl -n hw.ncpu); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Build type: $(BUILD_TYPE)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e BUILD_THREADS=$$BUILD_THREADS \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"clang --version && \
		cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $$(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"$(BUILD_DIR)\" --target generated -- -j${BUILD_THREADS} && \
		cd /opt/kagome/ && export CI='$(CI)' && ./housekeeping/clang-tidy-diff.sh \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME

kagome_dev_docker_build_sanitizers:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	CONTAINER_NAME=kagome_dev_build_$$(openssl rand -hex 6); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_THREADS=$$(nproc 2>/dev/null || sysctl -n hw.ncpu); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Build type: $(BUILD_TYPE)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e BUILD_THREADS=$$BUILD_THREADS \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-e UBSAN_OPTIONS=$(UBSAN_OPTIONS) \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"clang --version && \
		cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $$(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -DBACKWARD=OFF -DWERROR=$(WERROR) $(SAN_PARAMS) && \
		cmake --build \"$(BUILD_DIR)\" --target test -- -j${BUILD_THREADS} \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME

kagome_dev_docker_build_tidy:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	CONTAINER_NAME=kagome_dev_build_$$(openssl rand -hex 6); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_THREADS=$$(nproc 2>/dev/null || sysctl -n hw.ncpu); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Build type: $(BUILD_TYPE)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e BUILD_THREADS=$$BUILD_THREADS \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"clang --version && \
		cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $$(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"$(BUILD_DIR)\" --target generated -- -j${BUILD_THREADS} && \
		cd /opt/kagome/ && export CI='$(CI)' && ./housekeeping/clang-tidy-diff.sh \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME

upload_apt_package:
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev_$$(date +'%y.%m.%d')-$${SHORT_COMMIT_HASH}-$(BUILD_TYPE)_$(PACKAGE_ARCHITECTURE).deb ; \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ] && [ "$(GIT_REF_NAME)" != "master" ] && [ "$(BUILD_TYPE)" = "Release" ]; then \
		gcloud artifacts apt upload $(PUBLIC_ARTIFACTS_REPO) --source=./pkg/kagome_$(KAGOME_SANITIZED_VERSION)-$(BUILD_TYPE)_$(PACKAGE_ARCHITECTURE).deb ; \
	fi;

runtime_cache:
	CONTAINER_NAME=kagome_dev_runtime_cache_$$(openssl rand -hex 6); \
	RUNTIME_VERSION=$$(date +'%y.%m.%d')-$$(python3 get_wasmedge_version.py); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Runtime version: $$RUNTIME_VERSION"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e RUNTIME_VERSION=$$RUNTIME_VERSION \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GOOGLE_APPLICATION_CREDENTIALS=/root/.gcp/google_creds.json \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		$(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome/zombienet && \
		sed -i '1s/^#//' /etc/apt/sources.list.d/kagome.list && \
		install_packages kagome-dev && \
		./precompile.sh && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		mkdir -p /tmp/kagome_runtime && \
		cp -r /tmp/kagome/runtimes-cache/* /tmp/kagome_runtime && \
		./build_apt_package.sh \
			\"$${RUNTIME_VERSION}\" \
			$(PACKAGE_ARCHITECTURE) \
			kagome-dev-runtime \
			/tmp/kagome_runtime \
			'Kagome Runtime Dev Ubuntu Package' \
			'kagome-dev' \
			 /tmp/kagome/runtimes-cache/ ; \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME

upload_apt_package_runtime:
	RUNTIME_VERSION=$$(date +'%y.%m.%d')-$$(python3 get_wasmedge_version.py); \
	echo "Runtime version: $$RUNTIME_VERSION"; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	RUNTIME_PACKAGE_EXIST=$$(gcloud artifacts versions list --package=kagome-dev-runtime --format=json | jq -e ".[] | select(.name | endswith(\"${RUNTIME_VERSION}\"))" > /dev/null && echo "True" || echo "False"); \
	if [ "$$RUNTIME_PACKAGE_EXIST" = "True" ]; then \
		echo "Package with version $$RUNTIME_VERSION already exists"; \
		gcloud artifacts versions delete $$RUNTIME_VERSION --package=kagome-dev-runtime --quiet; \
	fi; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev-runtime_$${RUNTIME_VERSION}_$(PACKAGE_ARCHITECTURE).deb

kagome_image_build:
	$(MAKE) get_versions; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	BUILD_TARGET=""; \
	if [ "$(BUILD_TYPE)" = "Debug" ] || [ "$(BUILD_TYPE)" = "RelWithDebInfo" ]; then \
		BUILD_TARGET="--target debug"; \
	fi; \
	docker build --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
		-t $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest \
		--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
		-f kagome_runner.Dockerfile \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
		--build-arg ARCHITECTURE=$(ARCHITECTURE) \
		--build-arg KAGOME_PACKAGE_VERSION=$(KAGOME_PACKAGE_VERSION) \
		--build-arg PROJECT_ID=$(PROJECT_ID) \
		$${BUILD_TARGET} .

kagome_image_push:
	BUILD_TYPE_LOWER=$$(echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} ; \
	echo "KAGOME_IMAGE=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH}" | tee /tmp/docker_image.env ; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest ; \
	echo "KAGOME_IMAGE_LATEST=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:latest" | tee -a /tmp/docker_image.env ; \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}; \
			docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}; \
			echo "KAGOME_IMAGE_MASTER=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${MAIN_TAG}" | tee -a /tmp/docker_image.env ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}; \
			docker push $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}; \
			echo "KAGOME_IMAGE_TAG=$(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${GIT_REF_NAME}" | tee -a /tmp/docker_image.env ; \
		fi \
	fi

kagome_image_push_dockerhub:
	BUILD_TYPE_LOWER="release" ; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ]; then \
		if [ "$(GIT_REF_NAME)" = "master" ]; then \
			MAIN_TAG="$${SHORT_COMMIT_HASH}-master"; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):master; \
			docker push $(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}; \
			echo "KAGOME_DOCKERHUB_IMAGE_MASTER=$(DOCKERHUB_REGISTRY_PATH):$${MAIN_TAG}" | tee /tmp/dockerhub_image.env ; \
			docker push $(DOCKERHUB_REGISTRY_PATH):master; \
			echo "KAGOME_DOCKERHUB_IMAGE_LATEST=$(DOCKERHUB_REGISTRY_PATH):master" | tee -a /tmp/dockerhub_image.env ; \
		elif [ -n "$(GIT_REF_NAME)" ]; then \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}; \
			docker tag $(DOCKER_REGISTRY_PATH)kagome_$${BUILD_TYPE_LOWER}:$${SHORT_COMMIT_HASH} \
			$(DOCKERHUB_REGISTRY_PATH):latest; \
			docker push $(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}; \
			echo "KAGOME_DOCKERHUB_IMAGE_TAG=$(DOCKERHUB_REGISTRY_PATH):$${GIT_REF_NAME}" | tee /tmp/dockerhub_image.env ; \
			docker push $(DOCKERHUB_REGISTRY_PATH):latest; \
			echo "KAGOME_DOCKERHUB_IMAGE_LATEST=$(DOCKERHUB_REGISTRY_PATH):latest" | tee -a /tmp/dockerhub_image.env ; \
		fi \
	fi
	