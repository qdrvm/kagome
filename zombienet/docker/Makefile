BRANCH_NAME ?= master
KAGOME_IMAGE ?= qdrvm/kagome:latest
ZOMBIE_TESTER_IMAGE ?= qdrvm/zombie-tester:latest
ZOOMBIE_BUILDER_IMAGE ?= qdrvm/zombie-builder:testing
CONTAINER_NAME ?= zombienet-test
POLKADOT_BINARY_REPO ?= qdrvm/zombie-builder

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

get_versions:
	python3 version.py https://github.com/paritytech/polkadot-sdk && \
	python3 version.py https://github.com/paritytech/zombienet

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

builder_push:
	if [ -f polkadot-sdk-versions.txt ]; then \
  		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		docker push $(POLKADOT_BINARY_REPO)/polkadot_builder:$$POLKADOT_SDK_RELEASE ; \
		docker push $(POLKADOT_BINARY_REPO)/polkadot_builder:latest ; \
	else \
		echo "One or more files are missing."; \
	fi	

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
