#RUST_IMAGE=rust:1.79-slim-bookworm

RUST_VERSION ?= 1.79.0
# bookworm (Debian 12)
MINIDEB_IMAGE ?= bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16
KAGOME_PACKAGE_VERSION ?= 20.07.24-0fa54d1-Release

PLATFORM ?= linux/amd64
ARCHITECTURE ?= x86_64
PACKAGE_ARCHITECTURE ?= amd64

DOCKER_REGISTRY_PATH ?=
ARTIFACTS_REPO ?= kagome-apt
REGION ?= europe-north1

MINIDEB_HASH := $(subst bitnami/minideb@sha256:,,$(MINIDEB_IMAGE))
MINIDEB_SHORT_HASH := $(shell echo $(MINIDEB_HASH) | cut -c1-7)
MINIDEB_TAG = $(MINIDEB_SHORT_HASH)_rust-$(RUST_VERSION)

DEPENDENCIES ?= libstdc++6, zlib1g, libgcc-s1, libc6, libtinfo6, libseccomp2, libatomic1

GITHUB_HUNTER_USERNAME ?=
GITHUB_HUNTER_TOKEN ?=
CTEST_OUTPUT_ON_FAILURE ?= 1

# Debug, Release, RelWithDebInfo
BUILD_TYPE ?= Release

BUILD_DIR ?= build
CACHE_DIR := $(shell pwd)/../../../../kagome/$(BUILD_DIR)/cache
WORKING_DIR := $(shell pwd)/../../../../kagome

export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto


get_versions:
	echo "full_commit_hash: `git rev-parse HEAD`" > commit_hash.txt
	echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" >> commit_hash.txt

kagome_builder_deb:
	docker build --platform $(PLATFORM) \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(MINIDEB_TAG) \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:latest \
    		-f kagome_builder_deb.Dockerfile \
    		--build-arg RUST_VERSION=$(RUST_VERSION) \
    		--build-arg BASE_IMAGE=$(MINIDEB_IMAGE) \
    		--build-arg ARCHITECTURE=$(ARCHITECTURE) .

kagome_builder_deb_push:
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(MINIDEB_TAG) ; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:latest

kagome_dev_docker_build:
	mkdir -p $(CACHE_DIR)/.cargo $(CACHE_DIR)/.hunter $(CACHE_DIR)/.cache/ccache $(CACHE_DIR)/.rustup ; \
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
		-v $(CACHE_DIR)/.cargo:/root/.cargo \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		-v $(CACHE_DIR)/.rustup:/root/.rustup \
		$(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(MINIDEB_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF && \
		cmake --build \"$(BUILD_DIR)\" --target kagome -- -j${BUILD_THREADS} && \
		mkdir -p /tmp/kagome && \
		cp /opt/kagome/$(BUILD_DIR)/node/kagome /tmp/kagome/kagome && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		./build_apt_package.sh \
			\"$$(date +'%y.%m.%d')-$${SHORT_COMMIT_HASH}-$(BUILD_TYPE)\" \
			$(PACKAGE_ARCHITECTURE) \
			kagome-dev \
			/tmp/kagome \
			'Kagome Dev Debian Package' \
			'$(DEPENDENCIES)' ; \
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
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev_$$(date +'%y.%m.%d')-$${SHORT_COMMIT_HASH}-$(BUILD_TYPE)_$(PACKAGE_ARCHITECTURE).deb
	

runtime_cache:
	CONTAINER_NAME=kagome_dev_runtime_cache_$$(openssl rand -hex 6); \
	RUNTIME_VERSION=$$(python3 get_wasmedge_version.py)-$$(date +'%y.%m.%d'); \
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
		$(DOCKER_REGISTRY_PATH)zombie_tester:latest \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome/zombienet && \
		sed -i '1s/^#//' /etc/apt/sources.list.d/kagome.list && \
		install_packages kagome-dev=$(KAGOME_PACKAGE_VERSION) && \
		./precompile.sh && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		mkdir -p /tmp/kagome_runtime && \
		cp -r /tmp/kagome/runtimes-cache/* /tmp/kagome_runtime && \
		./build_apt_package.sh \
			\"$${RUNTIME_VERSION}\" \
			$(PACKAGE_ARCHITECTURE) \
			kagome-dev-runtime \
			/tmp/kagome_runtime \
			'Kagome Runtime Dev Debian Package' \
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
	RUNTIME_VERSION=$$(python3 get_wasmedge_version.py); \
	echo "Runtime version: $$RUNTIME_VERSION"; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	RUNTIME_PACKAGE_EXIST=$$(gcloud artifacts versions list --package=kagome-dev-runtime --format=json | jq -e ".[] | select(.name | endswith(\"${RUNTIME_VERSION}\"))" > /dev/null && echo "True" || echo "False"); \
	if [ "$$RUNTIME_PACKAGE_EXIST" = "True" ]; then \
		echo "Package with version $$RUNTIME_VERSION already exists"; \
		gcloud artifacts versions delete $$RUNTIME_VERSION --package=kagome-dev-runtime --quiet; \
	fi; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev-runtime_$${RUNTIME_VERSION}_$(PACKAGE_ARCHITECTURE).deb