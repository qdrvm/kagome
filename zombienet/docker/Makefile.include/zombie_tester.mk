zombie_tester:
	if [ -f polkadot-sdk-versions.txt ]; then \
  		ZOMBIENET_RELEASE=$$(grep 'short_version:' zombienet-versions.txt | cut -d ' ' -f 2); \
		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		PROJECT_ID=$(PROJECT_ID) \
		docker build \
			--platform $(PLATFORM) \
			--no-cache \
			-t $(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG) \
			-t $(DOCKER_REGISTRY_PATH)zombie_tester:$${POLKADOT_SDK_RELEASE}_$${ZOMBIENET_RELEASE} \
			--secret id=google_creds,src=$(GOOGLE_APPLICATION_CREDENTIALS) \
			-f zombie_tester.Dockerfile \
			--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
			--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) \
			--build-arg PROJECT_ID=$$PROJECT_ID \
			--build-arg RUST_VERSION=$(RUST_VERSION) \
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
		docker push $(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG) ; \
	else \
		echo "-- One or more files are missing."; \
	fi

copy_logs_to_host:
	@CONTAINER_NAME=$(CONTAINER_NAME); \
	FINISHED_CONTAINER_NAME=$$CONTAINER_NAME-finished; \
	FINISHED_IMAGE_NAME=$$CONTAINER_NAME-finished-image; \
	echo "-- Copying logs from container $$CONTAINER_NAME to host path $(HOST_LOGS_PATH)"; \
	docker commit $$CONTAINER_NAME $$FINISHED_IMAGE_NAME; \
	echo "-- Starting temporary container $$FINISHED_CONTAINER_NAME to copy logs"; \
	docker run -d --name $$FINISHED_CONTAINER_NAME --platform $(PLATFORM) --entrypoint "/bin/bash" $$FINISHED_IMAGE_NAME -c "tail -f /dev/null"; \
	mkdir -p $(HOST_LOGS_PATH); \
	DIRS_TO_COPY=$$(docker exec $$FINISHED_CONTAINER_NAME "/bin/bash" -c "find /tmp/ -type d -name 'zombie-*'"); \
	for DIR in $$DIRS_TO_COPY; do \
		docker cp "$$FINISHED_CONTAINER_NAME:$$DIR/logs" "$(HOST_LOGS_PATH)/$$(basename $$DIR)"; \
	done; \
	docker cp "$$FINISHED_CONTAINER_NAME:/tmp/versions.env" "/tmp/versions.env" ; \
	echo "-- Logs copied to $(HOST_LOGS_PATH)"; \
	echo "-- Runtime cache directory content:"; \
	docker exec $$FINISHED_CONTAINER_NAME "/bin/bash" -c "ls -la /tmp/kagome/runtimes-cache/" ; \
	echo "-- Stop and removing container $$FINISHED_CONTAINER_NAME and image $$FINISHED_IMAGE_NAME"; \
	docker stop $$FINISHED_CONTAINER_NAME; \
	docker rm -f $$FINISHED_CONTAINER_NAME; \
	docker rmi $$FINISHED_IMAGE_NAME
