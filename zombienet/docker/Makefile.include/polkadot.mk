export POLKADOT_SDK_RELEASE

polkadot_builder:
	docker build --progress=plain --platform $(PLATFORM) \
		-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE) \
		-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
		-f polkadot_builder.Dockerfile \
		--build-arg RUST_VERSION=$(RUST_VERSION) \
		--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
		--build-arg SCCACHE_VERSION=$(SCCACHE_VERSION) \
		--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) .

polkadot_builder_push: set_versions
	docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(POLKADOT_SDK_RELEASE) ; \
	docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) ; \

polkadot_binary:
	$(MAKE) docker_run; \
	$(MAKE) docker_exec || $(MAKE) docker_stop_clean; \
	$(MAKE) docker_stop_clean

docker_run: set_versions docker_start_clean
	docker run -d --name $(POLKADOT_BUILD_CONTAINER_NAME) \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e RUSTC_WRAPPER=sccache \
		-e SCCACHE_VERSION=$(SCCACHE_VERSION) \
		-e SCCACHE_GCS_BUCKET=$(SCCACHE_GCS_BUCKET) \
		-e SCCACHE_GCS_KEY_PATH=/root/.gcp/google_creds.json \
		-e SCCACHE_GCS_KEY_PREFIX=$(SCCACHE_GCS_KEY_PREFIX) \
		-e SCCACHE_GCS_RW_MODE=READ_WRITE \
		-e SCCACHE_LOG=info \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e POLKADOT_SDK_RELEASE=$(POLKADOT_SDK_RELEASE) \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		-v ./$(DOCKER_BUILD_DIR_NAME)/polkadot_binary:/tmp/polkadot_binary \
		-v ./$(DOCKER_BUILD_DIR_NAME)/cargo/registry:/root/.cargo/registry/ \
		-v ./$(DOCKER_BUILD_DIR_NAME)/cargo/git:/root/.cargo/git/ \
		-v ./$(DOCKER_BUILD_DIR_NAME)/home:/home/nonroot/ \
		-v ./$(DOCKER_BUILD_DIR_NAME)/tmp:/tmp/ \
		-v ./build_apt_package.sh:/home/nonroot/build_apt_package.sh \
		$(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
		-c "tail -f /dev/null"

docker_exec: set_versions
	docker exec -t $(POLKADOT_BUILD_CONTAINER_NAME) /bin/bash -c \
		"if [ ! -d \"$(POLKADOT_REPO_DIR)/.git\" ]; then \
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
		for cmd in \"$(BUILD_COMMANDS)\"; do \
			echo \"Executing: $$cmd\"; \
			eval \"$$cmd\"; \
			if [ $$? -ne 0 ]; then \
				echo \"Error in command: $$cmd\"; \
				exit 1; \
			fi; \
		done && \
		cp $(RESULT_BINARIES_WITH_PATH) /tmp/polkadot_binary/ && \
		cd ~ && ./build_apt_package.sh \
			$(POLKADOT_RELEASE_GLOBAL_NUMERIC)-$(CURRENT_DATE) \
			$(ARCHITECTURE) \
			polkadot-binary \
			/tmp/polkadot_binary \
			'Polkadot binaries: $(RESULT_BIN_NAMES)' \
			'$(POLKADOT_BINARY_DEPENDENCIES)'"

docker_stop_clean:
	docker stop $(POLKADOT_BUILD_CONTAINER_NAME) || true

docker_start_clean:
	docker stop $(POLKADOT_BUILD_CONTAINER_NAME) || true
	docker rm $(POLKADOT_BUILD_CONTAINER_NAME) || true

reset_build_state: docker_start_clean
	rm ./commit_hash.txt ./kagome_version.txt ./polkadot-sdk-versions.txt ./zombienet-versions.txt || true
	rm -r ./build_docker || true

upload_apt_package:
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $$POLKADOT_RELEASE_GLOBAL_NUMERIC-$$SHORT_COMMIT_HASH --package=polkadot-binary --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/polkadot-binary_$$POLKADOT_RELEASE_GLOBAL_NUMERIC-$${SHORT_COMMIT_HASH}_$(ARCHITECTURE).deb
