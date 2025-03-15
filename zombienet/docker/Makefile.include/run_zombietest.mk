define RUNTIME_PACKAGE_VERSION
$(if $(RUNTIME_PACKAGE_VERSION_NOARCH),=$(RUNTIME_PACKAGE_VERSION_NOARCH)-$(ARCHITECTURE))
endef

# run_test takes arguments:
# (1) test file path
define run_test
	@CONTAINER_NAME=zombietest_$$(openssl rand -hex 6); \
	echo "Running test in container $$CONTAINER_NAME with params \n test:$(1) and \n Kagome package version: $(KAGOME_PACKAGE_VERSION)"; \
	START_TIME=$$(date +%s); \
	TEST_PATH=$$(echo $(1) | xargs); \
	trap 'echo "Caught interrupt signal, cleaning up container $$CONTAINER_NAME"; docker rm -f $$CONTAINER_NAME 2>/dev/null || true; exit 1' INT TERM EXIT; \
	docker run --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-v $(WORKING_DIR):/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e USER_NAME=kagome_builder \
		-e USER_GROUP=kagome_builder \
		-e TEST_PATH=$$TEST_PATH \
		-e MOUNTED_DIRS="/tmp/kagome" \
		-e KAGOME_PACKAGE_VERSION=$(KAGOME_PACKAGE_VERSION) \
		-e RUNTIME_PACKAGE_VERSION=$(RUNTIME_PACKAGE_VERSION) \
		-e GOOGLE_APPLICATION_CREDENTIALS=/root/.gcp/google_creds.json \
		$(ZOMBIE_TESTER_IMAGE) \
		-c "sed -i '1s/^#//' /etc/apt/sources.list.d/kagome.list && \
			install_packages kagome-dev=$(KAGOME_PACKAGE_VERSION) kagome-dev-runtime$(RUNTIME_PACKAGE_VERSION) && \
			echo \"KAGOME_DEV_VERSION=\$$(apt-cache policy kagome-dev | grep 'Installed:' | awk '{print \$$2}')\" && \
			echo \"KAGOME_DEV_RUNTIME_VERSION=\$$(apt-cache policy kagome-dev-runtime | grep 'Installed:' | awk '{print \$$2}')\" && \
			echo \"POLKADOT_VERSION=\$$(polkadot --version | awk '{print \$$2}')\" && \
			echo \"ZOMBIENET_VERSION=\$$(zombienet version)\" && \
			/usr/local/bin/entrypoint.sh \
			zombienet test -p native /opt/$$TEST_PATH " || TEST_EXIT_CODE=$$?; \
	trap - INT TERM EXIT; \
	ACTUAL_EXIT_CODE=$${TEST_EXIT_CODE:-$$(docker inspect $$CONTAINER_NAME --format='{{.State.ExitCode}}')}; \
	if [ "$(COPY_LOGS_TO_HOST)" = "true" ]; then \
		$(MAKE) copy_logs_to_host CONTAINER_NAME=$$CONTAINER_NAME; \
	fi; \
	END_TIME=$$(date +%s); \
	DURATION=$$((END_TIME - START_TIME)); \
	MINUTES=$$((DURATION / 60)); \
	SECONDS=$$((DURATION % 60)); \
	echo "Test duration: $$MINUTES min $$SECONDS sec"; \
	echo "Test finished with exit code $$ACTUAL_EXIT_CODE"; \
	echo "Runtime cache directory content:"; \
	if [ "$(DELETE_IMAGE_AFTER_TEST)" = "true" ]; then \
		docker rm -f $$CONTAINER_NAME 2>/dev/null || true; \
		echo "Container $$CONTAINER_NAME removed"; \
	fi; \
	exit $$ACTUAL_EXIT_CODE
endef

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
	echo "-- Logs copied to $(HOST_LOGS_PATH)"; \
	echo "-- Runtime cache directory content:"; \
	docker exec $$FINISHED_CONTAINER_NAME "/bin/bash" -c "ls -la /tmp/kagome/runtimes-cache/" ; \
	echo "-- Stop and removing container $$FINISHED_CONTAINER_NAME and image $$FINISHED_IMAGE_NAME"; \
	docker stop $$FINISHED_CONTAINER_NAME; \
	docker rm -f $$FINISHED_CONTAINER_NAME; \
	docker rmi $$FINISHED_IMAGE_NAME
