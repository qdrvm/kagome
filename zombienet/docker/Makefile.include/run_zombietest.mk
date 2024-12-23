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
