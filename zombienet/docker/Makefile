# Shared Environment Variables
# trixie (Debian 13)
MINIDEB_IMAGE ?= debian:trixie-slim@sha256:c63174f799a74411a0290f25086fdc5dc939aa1f8971a16ad6fb1b7658a25126
## DOCKER_REGISTRY_PATH empty for local usage
DOCKER_REGISTRY_PATH ?=
GOOGLE_APPLICATION_CREDENTIALS ?=
PLATFORM ?= linux/amd64
ARCHITECTURE ?= amd64

# polkadot_builder Variables
POLKADOT_SDK_TAG ?=
RUST_IMAGE=rust:1.81-slim-bookworm
SCCACHE_VERSION ?= 0.7.4

# polkadot_binary Variables
SCCACHE_GCS_BUCKET ?=
CARGO_PACKETS=-p test-parachain-adder-collator -p polkadot-test-malus -p test-parachain-undying-collator
RESULT_BINARIES=./target/release/malus ./target/release/undying-collator ./target/release/adder-collator
RESULT_BIN_NAMES=malus undying-collator adder-collator
POLKADOT_BINARY_DEPENDENCIES=libstdc++6, zlib1g, libgcc-s1, libc6

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
ZOMBIE_TESTER_IMAGE ?= zombie_tester:latest
KAGOME_PACKAGE_VERSION ?=
WORKING_DIR := $(shell pwd)/../../../kagome
DELETE_IMAGE_AFTER_TEST ?= true


export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto


# run_test takes two arguments:
# (1) docker image name
# (2) test file path
define run_test
	@CONTAINER_NAME=$$(openssl rand -hex 6); \
	echo "Running test in container $$CONTAINER_NAME with params \n image:$(1), test:$(2) and \n Kagome package version: $(KAGOME_PACKAGE_VERSION)"; \
	START_TIME=$$(date +%s); \
	TEST_PATH=$$(echo $(2) | xargs); \
	docker run --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-v $(WORKING_DIR):/home/nonroot/bin/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		-e TEST_PATH=$$TEST_PATH \
		-e KAGOME_PACKAGE_VERSION=$(KAGOME_PACKAGE_VERSION) \
		-e GOOGLE_APPLICATION_CREDENTIALS=/root/.gcp/google_creds.json \
		$(1) \
		-c "sed -i '1s/^#//' /etc/apt/sources.list.d/kagome.list && \
			install_packages kagome-dev=$(KAGOME_PACKAGE_VERSION) kagome-dev-runtime && \
			echo \"KAGOME_DEV_VERSION=\$$(apt-cache policy kagome-dev | grep 'Installed:' | awk '{print \$$2}')\" > /tmp/versions.env && \
			echo \"KAGOME_DEV_RUNTIME_VERSION=\$$(apt-cache policy kagome-dev-runtime | grep 'Installed:' | awk '{print \$$2}')\" >> /tmp/versions.env && \
			echo \"POLKADOT_VERSION=\$$(polkadot --version | awk '{print \$$2}')\" >> /tmp/versions.env && \
			echo \"ZOMBIENET_VERSION=\$$(zombienet version)\" >> /tmp/versions.env && \
			cat /tmp/versions.env && \
			zombienet-linux-x64 test -p native /home/nonroot/bin/$$TEST_PATH " ; \
	TEST_EXIT_CODE=$$(docker inspect $$CONTAINER_NAME --format='{{.State.ExitCode}}'); \
	if [ "$(COPY_LOGS_TO_HOST)" = "true" ]; then \
		$(MAKE) copy_logs_to_host CONTAINER_NAME=$$CONTAINER_NAME; \
	fi; \
	END_TIME=$$(date +%s); \
	DURATION=$$((END_TIME - START_TIME)); \
	MINUTES=$$((DURATION / 60)); \
	SECONDS=$$((DURATION % 60)); \
	echo "Test duration: $$MINUTES min $$SECONDS sec"; \
	echo "Test finished with exit code $$TEST_EXIT_CODE"; \
	echo "Runtime cache directory content:"; \
	if [ "$(DELETE_IMAGE_AFTER_TEST)" = "true" ]; then \
		docker rm -f $$CONTAINER_NAME; \
		echo "Container $$CONTAINER_NAME removed"; \
	fi; \
	exit $$TEST_EXIT_CODE
endef

get_versions:
	python3 version.py https://github.com/paritytech/polkadot-sdk $(POLKADOT_SDK_TAG) && \
	python3 version_sem.py https://github.com/paritytech/zombienet
	@echo "full_commit_hash: `git rev-parse HEAD`" | tee commit_hash.txt
	@echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" | tee -a commit_hash.txt
	@echo "kagome_version: `cd $(WORKING_DIR) && ./get_version.sh`" | tee kagome_version.txt

polkadot_builder:
	$(MAKE) get_versions
	if [ -f polkadot-sdk-versions.txt ]; then \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		docker build --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$$POLKADOT_SDK_RELEASE \
		-t $(DOCKER_REGISTRY_PATH)polkadot_builder:latest \
		-f polkadot_builder.Dockerfile \
		--build-arg RUST_IMAGE=$(RUST_IMAGE) \
		--build-arg SCCACHE_VERSION=$(SCCACHE_VERSION) \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE . ; \
	else \
		echo "One or more files are missing."; \
	fi

polkadot_builder_push:
	if [ -f polkadot-sdk-versions.txt ]; then \
  		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$$POLKADOT_SDK_RELEASE ; \
		docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:latest ; \
	else \
		echo "One or more files are missing."; \
	fi

polkadot_binary:
	if [ -f polkadot-sdk-versions.txt ]; then \
		CONTAINER_NAME=polkadot_build_$$(openssl rand -hex 6); \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
		DOCKER_EXEC_RESULT=0 ; \
		mkdir -p ./polkadot_binary ./cargo/registry ./cargo/git ./pkg; \
		docker run -d --name $$CONTAINER_NAME \
			--platform $(PLATFORM) \
			--entrypoint "/bin/bash" \
			-e RUSTC_WRAPPER=sccache \
			-e SCCACHE_GCS_BUCKET=$(SCCACHE_GCS_BUCKET) \
			-e SCCACHE_GCS_KEY_PATH=/root/.gcp/google_creds.json \
			-e SCCACHE_GCS_KEY_PREFIX=polkadot_builder \
			-e SCCACHE_GCS_RW_MODE=READ_WRITE \
			-e SCCACHE_LOG=info \
			-e ARCHITECTURE=$(ARCHITECTURE) \
			-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
			-e POLKADOT_RELEASE_GLOBAL_NUMERIC=$$POLKADOT_RELEASE_GLOBAL_NUMERIC \
			-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
			-v ./polkadot_binary:/tmp/polkadot_binary \
			-v ./cargo/registry:/usr/local/cargo/registry/ \
			-v ./cargo/git:/usr/local/cargo/git/ \
			-v ./pkg:/home/nonroot/polkadot-sdk/pkg \
			-v ./build_apt_package.sh:/home/nonroot/polkadot-sdk/build_apt_package.sh \
			$(DOCKER_REGISTRY_PATH)polkadot_builder:$$POLKADOT_SDK_RELEASE \
			-c "tail -f /dev/null"; \
		docker exec -t $$CONTAINER_NAME /bin/bash -c \
			"cargo update && \
			cargo build --release $(CARGO_PACKETS) && \
			cp $(RESULT_BINARIES) /tmp/polkadot_binary/ && \
		 	./build_apt_package.sh \
		 		$$POLKADOT_RELEASE_GLOBAL_NUMERIC-$$SHORT_COMMIT_HASH \
		 		$(ARCHITECTURE) \
		 		polkadot-binary \
		 		/tmp/polkadot_binary \
		 		'Polkadot binaries: $(RESULT_BIN_NAMES)' \
		 		'$(POLKADOT_BINARY_DEPENDENCIES)' ; \
		 		" || DOCKER_EXEC_RESULT=$$? ; \
		if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
			echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
			docker stop $$CONTAINER_NAME; \
			exit $$DOCKER_EXEC_RESULT; \
		fi; \
		docker stop $$CONTAINER_NAME; \
	else \
		echo "One or more files are missing."; \
	fi

upload_apt_package:
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $$POLKADOT_RELEASE_GLOBAL_NUMERIC-$$SHORT_COMMIT_HASH --package=polkadot-binary --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/polkadot-binary_$$POLKADOT_RELEASE_GLOBAL_NUMERIC-$${SHORT_COMMIT_HASH}_$(ARCHITECTURE).deb

zombie_tester:
	if [ -f polkadot-sdk-versions.txt ]; then \
  		ZOMBIENET_RELEASE=$$(grep 'short_version:' zombienet-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		PROJECT_ID=$(PROJECT_ID) \
		docker build \
			--platform $(PLATFORM) \
			--no-cache \
			-t $(DOCKER_REGISTRY_PATH)zombie_tester:latest \
			-t $(DOCKER_REGISTRY_PATH)zombie_tester:$${POLKADOT_SDK_RELEASE}_$${ZOMBIENET_RELEASE} \
			--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
			-f zombie_tester.Dockerfile \
			--build-arg BASE_IMAGE=$(MINIDEB_IMAGE) \
			--build-arg PROJECT_ID=$$PROJECT_ID \
			--build-arg POLKADOT_BINARY_PACKAGE_VERSION="$(POLKADOT_BINARY_PACKAGE_VERSION)" \
			--build-arg ZOMBIENET_RELEASE=$$ZOMBIENET_RELEASE \
			--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE . ; \
	else \
		echo "One or more files are missing."; \
	fi

zombie_tester_push:
	if [ -f polkadot-sdk-versions.txt ]; then \
		ZOMBIENET_RELEASE=$$(grep 'short_version:' zombienet-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		docker push $(DOCKER_REGISTRY_PATH)zombie_tester:$${POLKADOT_SDK_RELEASE}_$${ZOMBIENET_RELEASE} ; \
		docker push $(DOCKER_REGISTRY_PATH)zombie_tester:latest ; \
	else \
		echo "One or more files are missing."; \
	fi

copy_logs_to_host:
	@CONTAINER_NAME=$(CONTAINER_NAME); \
	FINISHED_CONTAINER_NAME=$$CONTAINER_NAME-finished; \
	FINISHED_IMAGE_NAME=$$CONTAINER_NAME-finished-image; \
	echo "Copying logs from container $$CONTAINER_NAME to host path $(HOST_LOGS_PATH)"; \
	docker commit $$CONTAINER_NAME $$FINISHED_IMAGE_NAME; \
	echo "Starting temporary container $$FINISHED_CONTAINER_NAME to copy logs"; \
	docker run -d --name $$FINISHED_CONTAINER_NAME --platform $(PLATFORM) --entrypoint "/bin/bash" $$FINISHED_IMAGE_NAME -c "tail -f /dev/null"; \
	mkdir -p $(HOST_LOGS_PATH); \
	DIRS_TO_COPY=$$(docker exec $$FINISHED_CONTAINER_NAME "/bin/bash" -c "find /tmp/ -type d -name 'zombie-*'"); \
	for DIR in $$DIRS_TO_COPY; do \
		docker cp "$$FINISHED_CONTAINER_NAME:$$DIR/logs" "$(HOST_LOGS_PATH)/$$(basename $$DIR)"; \
	done; \
	docker cp "$$FINISHED_CONTAINER_NAME:/tmp/versions.env" "/tmp/versions.env" ; \
	echo "Logs copied to $(HOST_LOGS_PATH)"; \
	echo "Runtime cache directory content:"; \
	docker exec $$FINISHED_CONTAINER_NAME "/bin/bash" -c "ls -la /tmp/kagome/runtimes-cache/" ; \
	echo "Stop and removing container $$FINISHED_CONTAINER_NAME and image $$FINISHED_IMAGE_NAME"; \
	docker stop $$FINISHED_CONTAINER_NAME; \
	docker rm -f $$FINISHED_CONTAINER_NAME; \
	docker rmi $$FINISHED_IMAGE_NAME

test-polkadot-functional-0001-parachains-pvf:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0001-parachains-pvf.zndsl")

test-polkadot-functional-0002-parachains-disputes:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0002-parachains-disputes.zndsl")

test-polkadot-functional-0003-beefy-and-mmr:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0003-beefy-and-mmr.zndsl")

test-polkadot-functional-0004-parachains-garbage-candidate:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0004-parachains-garbage-candidate.zndsl")

test-polkadot-functional-0005-parachains-disputes-past-session:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0005-parachains-disputes-past-session.zndsl")

test-polkadot-functional-0006-parachains-max-tranche0:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0006-parachains-max-tranche0.zndsl")

test-polkadot-functional-0007-dispute-freshly-finalized:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0007-dispute-freshly-finalized.zndsl")

test-polkadot-functional-0008-dispute-old-finalized:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0008-dispute-old-finalized.zndsl")

test-polkadot-functional-0009-approval-voting-coalescing:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0009-approval-voting-coalescing.zndsl")

test-polkadot-functional-0010-validator-disabling:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0010-validator-disabling.zndsl")

test-polkadot-functional-0011-async-backing-6-seconds-rate:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0011-async-backing-6-seconds-rate.zndsl")

test-polkadot-functional-0013-systematic-chunk-recovery:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/polkadot/functional/0013-systematic-chunk-recovery.zndsl")

test-custom-0001-validators-warp-sync:
	$(call run_test, $(ZOMBIE_TESTER_IMAGE), "kagome/zombienet/custom/0001-validators-warp-sync.zndsl")