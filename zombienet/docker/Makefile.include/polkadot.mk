polkadot_builder:
	docker build --progress=plain --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-$(ARCHITECTURE) \
		-f polkadot_builder.Dockerfile \
		--build-arg USER_ID=$(USER_ID) \
		--build-arg GROUP_ID=$(GROUP_ID) \
		--build-arg USER_NAME=$(IN_DOCKER_USERNAME) \
		--build-arg RUST_VERSION=$(RUST_VERSION) \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg SCCACHE_VERSION=$(SCCACHE_VERSION) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) .

polkadot_builder_push: set_versions
	docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-$(ARCHITECTURE) ; \

polkadot_builder_push_manifest:
	docker manifest create $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION) \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-arm64 && \
	docker manifest create $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-amd64 \
		--amend $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-arm64 && \
	docker manifest push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION) && \
	docker manifest push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG)

polkadot_builder_all_arch: set_versions
	$(MAKE) polkadot_builder PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) polkadot_builder_push PLATFORM=linux/amd64 ARCHITECTURE=amd64 && \
	$(MAKE) polkadot_builder PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) polkadot_builder_push PLATFORM=linux/arm64 ARCHITECTURE=arm64 && \
	$(MAKE) polkadot_builder_push_manifest

polkadot_builder_image_info: set_versions
	@echo "---------------------------------"
	@echo POLKADOT_BUILDER_IMAGE:       $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG)
	@echo POLKADOT_BUILDER_AMD64_IMAGE: $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-amd64
	@echo POLKADOT_BUILDER_ARM64_IMAGE: $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION)-arm64

polkadot_builder_check_tag:
	@docker manifest inspect $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE)-rust$(RUST_VERSION) > /dev/null 2>&1 && echo "true" || echo "false"

polkadot_binary:
	$(MAKE) docker_run; \
	$(MAKE) docker_exec || $(MAKE) docker_stop_clean; \
	$(MAKE) docker_stop_clean

docker_run: set_versions docker_start_clean
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
		tail -f /dev/null

docker_exec: set_versions
	docker exec -t $(POLKADOT_BUILD_CONTAINER_NAME) gosu $(IN_DOCKER_USERNAME) /bin/bash -c \
		"git config --global --add safe.directory \"*\" ; \
		env ; \
		echo \"-- Checking out Polkadot repository...\"; \
		if [ ! -d \"$(POLKADOT_REPO_DIR)/.git\" ]; then \
			mkdir -p $(POLKADOT_REPO_DIR) && \
			echo \"-- Cloning repository into $(POLKADOT_REPO_DIR)...\"; \
			git clone --depth 1 --branch $(POLKADOT_SDK_RELEASE) $(POLKADOT_REPO_URL) $(POLKADOT_REPO_DIR) ; \
			cd $(POLKADOT_REPO_DIR); \
		else \
			echo \"-- Updating repository...\"; \
			cd $(POLKADOT_REPO_DIR) && git fetch --tags --depth 1 origin && \
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
			$(POLKADOT_RELEASE_GLOBAL_NUMERIC)-$(CURRENT_DATE) \
			$(ARCHITECTURE) \
			polkadot-binary \
			$(IN_DOCKER_HOME)/polkadot_binary \
			'Polkadot binaries: $(RESULT_BIN_NAMES)' \
			'$(POLKADOT_BINARY_DEPENDENCIES)'"

docker_stop_clean:
	docker stop $(POLKADOT_BUILD_CONTAINER_NAME) || true

docker_start_clean: docker_stop_clean
	docker rm $(POLKADOT_BUILD_CONTAINER_NAME) || true

reset_build_state: docker_start_clean
	rm ./commit_hash.txt ./kagome_version.txt ./polkadot-sdk-versions.txt ./zombienet-versions.txt || true
	sudo rm -r ./$(DOCKER_BUILD_DIR_NAME) || true

upload_apt_package: set_versions
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $(POLKADOT_DEB_PACKAGE_VERSION) --package=polkadot-binary --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./$(DOCKER_BUILD_DIR_NAME)/pkg/$(POLKADOT_DEB_PACKAGE_NAME)

polkadot_deb_package_info: set_versions
	@echo "---------------------------------"
	@echo "POLKADOT_SDK_RELEASE:         $(POLKADOT_SDK_RELEASE)"
	@echo "POLKADOT_DEB_PACKAGE_NAME:    $(POLKADOT_DEB_PACKAGE_NAME)"
	@echo "POLKADOT_DEB_PACKAGE_VERSION: $(POLKADOT_DEB_PACKAGE_VERSION)"
