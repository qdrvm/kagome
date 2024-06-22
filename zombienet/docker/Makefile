RUST_IMAGE=rust:1.79-slim-bookworm
# bookworm (Debian 12)
MINIDEB_IMAGE=bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16

DOCKER_REGISTRY_PATH ?=
PLATFORM ?= linux/amd64
ARCH ?= amd64
SCCACHE_VERSION ?= 0.7.4
ARTIFACTS_REPO ?= kagome-apt
REGION ?= europe-north1

#USE_GCP_SCCACHE ?= true
SCCACHE_GCS_BUCKET ?= 
GOOGLE_APPLICATION_CREDENTIALS ?=

CARGO_PACKETS=-p test-parachain-adder-collator -p polkadot-test-malus -p test-parachain-undying-collator
RESULT_BINARIES=./target/testnet/malus ./target/testnet/undying-collator ./target/testnet/adder-collator
RESULT_BIN_NAMES=malus undying-collator adder-collator

export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto




BRANCH_NAME ?= master
KAGOME_IMAGE ?= qdrvm/kagome:latest
ZOMBIE_TESTER_IMAGE ?= qdrvm/zombie-tester:latest
ZOOMBIE_BUILDER_IMAGE ?= qdrvm/zombie-builder:testing
CONTAINER_NAME ?= zombienet-test

COPY_LOGS_TO_HOST ?= true
HOST_LOGS_PATH ?= /tmp/test_logs

CUMULUS_RELEASE ?= v0.9.420
POLKADOT_RELEASE ?= v0.9.42
POLKADOT_RELEASE_GLOBAL ?= v0.9.42

# run_test takes two arguments: 
# (1) docker image name
# (2) test file path
# COPY_LOGS_TO_HOST: boolean flag to determine whether to copy logs to host

define run_test
	@CONTAINER_NAME=$$(openssl rand -hex 6); \
	echo "Running test in container $$CONTAINER_NAME with params image:$(1), test:$(2)"; \
	docker run --name $$CONTAINER_NAME --platform linux/amd64 $(1) zombienet-linux-x64 test -p native $(2); \
	TEST_EXIT_CODE=$$(docker inspect $$CONTAINER_NAME --format='{{.State.ExitCode}}'); \
	if [ "$(COPY_LOGS_TO_HOST)" = "true" ]; then \
		$(MAKE) copy_logs_to_host CONTAINER_NAME=$$CONTAINER_NAME; \
	fi; \
	echo "Test finished with exit code $$TEST_EXIT_CODE"; \
	exit $$TEST_EXIT_CODE
endef

get_versions:
	python3 version.py https://github.com/paritytech/polkadot-sdk && \
	python3 version.py https://github.com/paritytech/zombienet
	echo "full_commit_hash: `git rev-parse HEAD`" > commit_hash.txt
	echo "short_commit_hash: `git rev-parse HEAD | head -c 7`" >> commit_hash.txt

polkadot_builder:
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
			-e ARCH=$(ARCH) \
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
			"cargo update $(CARGO_PACKETS) && \
			cargo build --profile testnet $(CARGO_PACKETS) && \
			mv -f $(RESULT_BINARIES) /tmp/polkadot_binary/ && \
		 	./build_apt_package.sh \
		 		$$POLKADOT_RELEASE_GLOBAL_NUMERIC-$$SHORT_COMMIT_HASH \
		 		$ARCH \
		 		polkadot-binary \
		 		/tmp/polkadot_binary \
		 		'Polkadot binaries: $(RESULT_BIN_NAMES)' \
		 		'libstdc++6, zlib1g, libgcc-s1, libc6' ; \
		 		ls -la ./polkadot_binary/ ./pkg/ ; \
		 		" || DOCKER_EXEC_RESULT=$$? ; \
		if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
			echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
			docker stop $$CONTAINER_NAME; \
			exit $$DOCKER_EXEC_RESULT; \
		fi; \
		docker stop $$CONTAINER_NAME; \
		ls -la ./polkadot_binary/ ./pkg/ ./cargo/registry/ ./cargo/git/ ; \
	else \
		echo "One or more files are missing."; \
	fi

upload_apt_package:
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
	ARCH=$(ARCH); \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $$POLKADOT_RELEASE_GLOBAL_NUMERIC-$$SHORT_COMMIT_HASH --package=polkadot-binary --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/polkadot-binary_$$POLKADOT_RELEASE_GLOBAL_NUMERIC-$${SHORT_COMMIT_HASH}_$$ARCH.deb


polkadot_binary_temp:
	if [ -f polkadot-sdk-versions.txt ]; then \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL=$$(grep 'short_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		CARGO_PACKETS="-p test-parachain-adder-collator -p polkadot-test-malus -p test-parachain-undying-collator" \
		DOCKER_BUILDKIT=0 docker build -t $(POLKADOT_BINARY_REPO)/polkadot_binary:$$POLKADOT_SDK_RELEASE -t $(POLKADOT_BINARY_REPO)/polkadot_binary:latest \
		-f polkadot_builder.Dockerfile \
		--build-arg ZOMBIENET_RELEASE=$(ZOMBIENET_RELEASE) \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE \
		--build-arg POLKADOT_RELEASE_GLOBAL=$$POLKADOT_RELEASE_GLOBAL \
		--build-arg POLKADOT_RELEASE_GLOBAL_NUMERIC=$$POLKADOT_RELEASE_GLOBAL_NUMERIC . ; \
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
	docker run -d --name $$FINISHED_CONTAINER_NAME --platform linux/amd64 --entrypoint "/bin/bash" $$FINISHED_IMAGE_NAME -c "tail -f /dev/null"; \
	mkdir -p $(HOST_LOGS_PATH); \
	DIRS_TO_COPY=$$(docker exec $$FINISHED_CONTAINER_NAME "/bin/bash" -c "find /tmp/ -type d -name 'zombie-*'"); \
	for DIR in $$DIRS_TO_COPY; do \
		docker cp "$$FINISHED_CONTAINER_NAME:$$DIR/logs" "$(HOST_LOGS_PATH)/$$(basename $$DIR)"; \
	done; \
	echo "Stop and removing container $$FINISHED_CONTAINER_NAME and image $$FINISHED_IMAGE_NAME"; \
	docker stop $$FINISHED_CONTAINER_NAME; \
	docker rm -f $$FINISHED_CONTAINER_NAME; \
	docker rmi $$FINISHED_IMAGE_NAME




fetch_and_run:
	if [ -f polkadot-sdk-versions.txt ]; then \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL=$$(grep 'short_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		ZOMBIENET_RELEASE=$$(grep 'short_version:' zombienet-versions.txt | cut -d ' ' -f 2); \
		DOCKER_BUILDKIT=0 docker build -t $(POLKADOT_BINARY_REPO)/polkadot_builder:$$POLKADOT_SDK_RELEASE -t $(POLKADOT_BINARY_REPO)/polkadot_builder:latest \
		-f polkadot_builder.Dockerfile \
		--build-arg ZOMBIENET_RELEASE=$$ZOMBIENET_RELEASE \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE \
		--build-arg POLKADOT_RELEASE_GLOBAL=$$POLKADOT_RELEASE_GLOBAL \
		--build-arg POLKADOT_RELEASE_GLOBAL_NUMERIC=$$POLKADOT_RELEASE_GLOBAL_NUMERIC . ; \
	else \
		echo "One or more files are missing."; \
	fi



build_polkadot_binary:
	IMAGE=$(POLKADOT_BINARY_REPO)/polkadot_builder:latest; \
	CARGO_PACKETS="-p test-parachain-adder-collator -p polkadot-test-malus -p test-parachain-undying-collator" \
	docker run --rm --platform linux/amd64 -v $(PWD):/workspace -w /workspace $$IMAGE cargo build --release $$CARGO_PACKETS



builder:
	DOCKER_BUILDKIT=0 docker build -t qdrvm/zombie-builder:latest -f builder.Dockerfile --build-arg CUMULUS_RELEASE=$(CUMULUS_RELEASE) --build-arg POLKADOT_RELEASE=$(POLKADOT_RELEASE) --build-arg POLKADOT_RELEASE_GLOBAL=$(POLKADOT_RELEASE_GLOBAL) .

tester:
	python3 version.py https://github.com/paritytech/polkadot-sdk && \
	if [ -f polkadot-sdk-versions.txt ]; then \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		BRANCH_NAME="$(BRANCH_NAME)" \
		KAGOME_IMAGE="$(KAGOME_IMAGE)" \
		DOCKER_BUILDKIT=0 docker build --platform linux/amd64 --no-cache -t qdrvm/zombie-tester:latest -t zombie-tester:latest -f tester.Dockerfile \
		--build-arg BRANCH_NAME=$$BRANCH_NAME \
		--build-arg KAGOME_IMAGE=$$KAGOME_IMAGE \
		--build-arg POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE . ; \
	else \
		echo "One or more files are missing."; \
	fi

tests:
	docker run --platform linux/amd64 -it qdrvm/zombie-tester:latest bash

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
