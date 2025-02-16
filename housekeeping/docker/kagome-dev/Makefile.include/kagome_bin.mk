kagome_docker_build:
	@echo "-- Building Kagome binary for $(ARCHITECTURE) architecture..." ; \
	$(MAKE) docker_run ; \
	if $(MAKE) docker_exec; then \
		echo "-- Build successful."; \
	else \
		echo "-- Build failed. Cleaning up..."; \
		$(MAKE) clean_container; \
		exit 1; \
	fi ; \
	$(MAKE) clean_container

stop_container:
	@echo "-- Stopping and removing polkadot_builder container..."
	docker stop $(CONTAINER_NAME) || true

clean_container: stop_container
	@echo "-- Removing previous build container..."
	docker rm $(CONTAINER_NAME) || true

reset_build_state: clean_container
	@echo "-- Resetting build state..." ; \
    rm -f ./commit_hash.txt ./kagome_version.txt || true; \
    $(call safe_rm,$(BUILD_DIR)) || true

docker_run: clean_container
	@echo "-- Running Kagome build container..." ; \
	mkdir -p \
		$(CACHE_DIR)/.cargo \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Build type: $(BUILD_TYPE)"; \
	docker run -d --name $(CONTAINER_NAME) \
		--platform $(PLATFORM) \
		-e SHORT_COMMIT_HASH=$(SHORT_COMMIT_HASH) \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-e MOUNTED_DIRS="/home/builder /opt/kagome" \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):$(IN_DOCKER_HOME)/.gcp/google_creds.json \
		-v ./build_apt_package.sh:$(IN_DOCKER_HOME)/build_apt_package.sh \
		-v $(WORKING_DIR):/opt/kagome \
		-v $(BUILD_DIR)/pkg:$(IN_DOCKER_HOME)/pkg \
		-v $(BUILD_DIR)/kagome_binary:$(IN_DOCKER_HOME)/kagome_binary \
		-v $(CACHE_DIR)/.cargo/registry:$(IN_DOCKER_HOME)/.cargo/registry \
		-v $(CACHE_DIR)/.cargo/git:$(IN_DOCKER_HOME)/.cargo/git \
		-v $(CACHE_DIR)/.hunter:$(IN_DOCKER_HOME)/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:$(IN_DOCKER_HOME)/.cache/ccache \
		$(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
		tail -f /dev/null ; \
	until [ "$$(docker inspect --format='{{.State.Health.Status}}' $(CONTAINER_NAME))" = "healthy" ]; do \
	  echo "Health is: \"$$(docker inspect --format='{{.State.Health.Status}}' $(CONTAINER_NAME))\"" ; \
	  sleep 5 ; \
	done

docker_exec:
	echo "-- Executing kagome_builder container..." ; \
	docker exec -t $(CONTAINER_NAME) gosu $(USER_ID):$(GROUP_ID) /bin/bash -c \
		"echo \"-- Setting up environment...\"; \
		env ; \
		cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory $(IN_DOCKER_HOME)/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"-- Recent commits:\" && git log --oneline -n 5 && \
		echo \"-- Building Kagome in $$(pwd)\" && \
		$(BUILD_COMMANDS) && \
		echo \"-- Copying Kagome binary...\" && \
		cp /opt/kagome/$(DOCKER_BUILD_DIR_NAME)/node/kagome $(IN_DOCKER_HOME)/kagome_binary/kagome && \
		echo \"-- Building apt package...\" && \
		cd $(IN_DOCKER_HOME) && ./build_apt_package.sh \
			$(KAGOME_DEB_PACKAGE_VERSION) \
			$(ARCHITECTURE) \
			kagome-dev \
			$(IN_DOCKER_HOME)/kagome_binary \
			'Kagome Dev Ubuntu Package' \
			'$(DEPENDENCIES)' && \
		if [ "$(IS_MAIN_OR_TAG)" = "true" ] && [ "$(GIT_REF_NAME)" != "master" ] && [ "$(BUILD_TYPE)" = "Release" ]; then \
			./build_apt_package.sh \
				$(KAGOME_DEB_RELEASE_PACKAGE_VERSION) \
				$(ARCHITECTURE) \
				kagome \
				$(IN_DOCKER_HOME)/kagome_binary \
				'Kagome Ubuntu Package' \
				'$(DEPENDENCIES)' ; \
		fi; \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		exit $$DOCKER_EXEC_RESULT; \
	fi

upload_apt_package:
	@echo "-- Uploading Kagome binary to Google Cloud Storage..." ; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $(KAGOME_DEB_PACKAGE_VERSION) --package=kagome-dev --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./$(BUILD_DIR)/pkg/$(KAGOME_DEB_PACKAGE_NAME) ; \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ] && [ "$(GIT_REF_NAME)" != "master" ] && [ "$(BUILD_TYPE)" = "Release" ]; then \
	  	echo "-- Uploading Kagome release binary to Google Cloud Storage..." ; \
	  	gcloud artifacts versions delete $(KAGOME_DEB_RELEASE_PACKAGE_VERSION) --package=kagome-dev --quiet || true ; \
		gcloud artifacts apt upload $(PUBLIC_ARTIFACTS_REPO) --source=./$(BUILD_DIR)/pkg/KAGOME_DEB_RELEASE_PACKAGE_NAME ; \
	fi;

kagome_deb_package_info:
	@echo "Kagome version: 						$(KAGOME_SANITIZED_VERSION)"
	@echo "Kagome deb package version: 			$(KAGOME_DEB_PACKAGE_VERSION)"
	@echo "Kagome deb package name: 			$(KAGOME_DEB_PACKAGE_NAME)"
	@echo "Kagome deb release package version: 	$(KAGOME_DEB_RELEASE_PACKAGE_VERSION)"
	@echo "Kagome deb release package name: 	$(KAGOME_DEB_RELEASE_PACKAGE_NAME)"
