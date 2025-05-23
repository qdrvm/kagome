polkadot_binary:
	echo "-- Building Polkadot binary for $(ARCHITECTURE) architecture..." ; \
	$(MAKE) docker_run ; \
	if $(MAKE) docker_exec; then \
		echo "-- Build successful."; \
	else \
		echo "-- Build failed. Cleaning up..."; \
		$(MAKE) clean_container; \
		exit 1; \
	fi ; \
	$(MAKE) clean_container

docker_run: clean_container
	echo "-- Running polkadot_builder container..." ; \
	echo "POLKADOT_SDK_RELEASE=$(POLKADOT_SDK_RELEASE)" ; \
	echo "POLKADOT_RELEASE_GLOBAL_NUMERIC=$(POLKADOT_RELEASE_GLOBAL_NUMERIC)" ; \
	mkdir -p ./$(DOCKER_BUILD_DIR_NAME)/cargo ; \
	docker run -d --name $(POLKADOT_BUILD_CONTAINER_NAME) \
		--platform $(PLATFORM) \
		--user root \
		-e RUSTC_WRAPPER=sccache \
		-e SCCACHE_GCS_RW_MODE=READ_WRITE \
		-e SCCACHE_VERSION=$(SCCACHE_VERSION) \
		-e SCCACHE_GCS_BUCKET=$(SCCACHE_GCS_BUCKET) \
		-e SCCACHE_GCS_KEY_PATH=$(IN_DOCKER_HOME)/.gcp/google_creds.json \
		-e SCCACHE_GCS_KEY_PREFIX=$(SCCACHE_GCS_KEY_PREFIX) \
		-e SCCACHE_GCS_RW_MODE=READ_WRITE \
		-e SCCACHE_LOG=info \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e POLKADOT_SDK_RELEASE=$(POLKADOT_SDK_RELEASE) \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):$(IN_DOCKER_HOME)/.gcp/google_creds.json \
		-v ./build_apt_package.sh:$(IN_DOCKER_HOME)/build_apt_package.sh \
		-v ./$(DOCKER_BUILD_DIR_NAME)/pkg:$(IN_DOCKER_HOME)/pkg \
		-v ./$(DOCKER_BUILD_DIR_NAME)/polkadot_binary:$(IN_DOCKER_HOME)/polkadot_binary \
		-v ./$(DOCKER_BUILD_DIR_NAME)/cargo/registry:$(IN_DOCKER_HOME)/.cargo/registry/ \
		-v ./$(DOCKER_BUILD_DIR_NAME)/cargo/git:$(IN_DOCKER_HOME)/.cargo/git/ \
		-v ./$(DOCKER_BUILD_DIR_NAME)/workspace:$(IN_DOCKER_HOME)/workspace \
		$(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
		tail -f /dev/null ; \
	until [ "$$(docker inspect --format='{{.State.Health.Status}}' $(POLKADOT_BUILD_CONTAINER_NAME))" = "healthy" ]; do \
	  echo "Health is: \"$$(docker inspect --format='{{.State.Health.Status}}' $(POLKADOT_BUILD_CONTAINER_NAME))\"" ; \
	  sleep 5 ; \
	done

docker_exec:
	echo "-- Executing polkadot_builder container..." ; \
	docker exec -t $(POLKADOT_BUILD_CONTAINER_NAME) gosu $(USER_ID):$(GROUP_ID) /bin/bash -c \
		"echo \"-- Setting up environment...\"; \
		env ; \
		echo \"-- Checking out Polkadot repository...\"; \
		if [ ! -d \"$(POLKADOT_REPO_DIR)/.git\" ]; then \
			ls -la && \
			mkdir -p $(POLKADOT_REPO_DIR) && \
			echo \"-- Cloning repository into $(POLKADOT_REPO_DIR)...\"; \
			git clone --no-progress --depth 1 --branch $(POLKADOT_SDK_RELEASE) $(POLKADOT_REPO_URL) $(POLKADOT_REPO_DIR) ; \
			cd $(POLKADOT_REPO_DIR); \
		else \
			echo \"-- Updating repository...\"; \
			cd $(POLKADOT_REPO_DIR) && git fetch --no-progress --tags --depth 1 origin && \
			git reset --hard HEAD && \
			git checkout $(POLKADOT_SDK_RELEASE) || git checkout -b $(POLKADOT_SDK_RELEASE) $(POLKADOT_SDK_RELEASE) && \
			git reset --hard $(POLKADOT_SDK_RELEASE); \
		fi && \
		echo \"-- Recent commits:\" && git log --oneline -n 5 && \
		echo \"-- Build polkadot...\" && \
		$(BUILD_COMMANDS) && \
		cp $(RESULT_BINARIES_WITH_PATH) $(IN_DOCKER_HOME)/polkadot_binary && \
		echo \"-- Building apt package...\" && \
		cd $(IN_DOCKER_HOME) && ./build_apt_package.sh \
			$(POLKADOT_DEB_PACKAGE_VERSION) \
			$(ARCHITECTURE) \
			polkadot-binary \
			$(IN_DOCKER_HOME)/polkadot_binary \
			'Polkadot binaries: $(RESULT_BIN_NAMES)' \
			'$(POLKADOT_BINARY_DEPENDENCIES)' ; \
			"  || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		exit $$DOCKER_EXEC_RESULT; \
	fi

stop_container:
	@echo "-- Stopping and removing polkadot_builder container..."
	docker stop $(POLKADOT_BUILD_CONTAINER_NAME) || true

clean_container: stop_container
	@echo "-- Removing previous build container..."
	docker rm $(POLKADOT_BUILD_CONTAINER_NAME) || true

reset_build_state: clean_container
	@echo "-- Resetting build state..."
	rm ./commit_hash.txt ./kagome_version.txt ./polkadot-sdk-versions.txt ./zombienet-versions.txt || true
	sudo rm -r ./$(DOCKER_BUILD_DIR_NAME) || true

upload_apt_package:
	echo "-- Uploading polkadot apt package to Google Cloud Storage..." ; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO) ; \
	gcloud config set artifacts/location $(REGION) ; \
	gcloud artifacts versions delete $(POLKADOT_DEB_PACKAGE_VERSION) --package=polkadot-binary --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./$(DOCKER_BUILD_DIR_NAME)/pkg/$(POLKADOT_DEB_PACKAGE_NAME)

polkadot_deb_package_info:
	@echo "-- Polkadot DEB Package Info --"
	@echo "POLKADOT_SDK_RELEASE:         $(POLKADOT_SDK_RELEASE)"
	@echo "POLKADOT_DEB_PACKAGE_NAME:    $(POLKADOT_DEB_PACKAGE_NAME)"
	@echo "POLKADOT_DEB_PACKAGE_VERSION: $(POLKADOT_DEB_PACKAGE_VERSION)"

.PHONY: polkadot_binary docker_run docker_exec stop_container clean_container reset_build_state
