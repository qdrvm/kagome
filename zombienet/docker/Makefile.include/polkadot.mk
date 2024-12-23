polkadot_builder:
	docker build --platform $(PLATFORM) \
	-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$(CURRENT_DATE) \
	-t $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
	-f polkadot_builder.Dockerfile \
	--build-arg RUST_VERSION=$(RUST_VERSION) \
	--build-arg BASE_IMAGE=$(OS_IMAGE_NAME) \
	--build-arg BASE_IMAGE_TAG=$(OS_IMAGE_TAG_WITH_HASH) .

polkadot_builder_push:
	if [ -f polkadot-sdk-versions.txt ]; then \
  		POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$$POLKADOT_SDK_RELEASE ; \
		docker push $(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) ; \
	else \
		echo "-- One or more files are missing."; \
	fi

polkadot_binary:
	$(MAKE) get_versions
	if [ -f polkadot-sdk-versions.txt ]; then \
		CONTAINER_NAME=polkadot_build_$$(openssl rand -hex 6); \
		if [ -z "$(POLKADOT_SDK_RELEASE)" ]; then \
		    POLKADOT_SDK_RELEASE=$$(grep 'polkadot_format_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
		fi; \
		echo "-- POLKADOT_SDK_RELEASE: $$POLKADOT_SDK_RELEASE"; \
		DOCKER_EXEC_RESULT=0; \
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
			-e POLKADOT_SDK_RELEASE=$$POLKADOT_SDK_RELEASE \
			-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
			-v ./$(DOCKER_BUILD_DIR_NAME)/polkadot_binary:/tmp/polkadot_binary \
			-v ./$(DOCKER_BUILD_DIR_NAME)/cargo/registry:/root/.cargo/registry/ \
			-v ./$(DOCKER_BUILD_DIR_NAME)/cargo/git:/root/.cargo/git/ \
			-v ./$(DOCKER_BUILD_DIR_NAME)/home:/home/nonroot/ \
			-v ./$(DOCKER_BUILD_DIR_NAME)/tmp:/tmp/ \
			-v ./build_apt_package.sh:/home/nonroot/build_apt_package.sh \
			$(DOCKER_REGISTRY_PATH)polkadot_builder:$(BUILDER_LATEST_TAG) \
			-c "tail -f /dev/null"; \
		docker exec -t $$CONTAINER_NAME /bin/bash -c \
			"if [ ! -d \"$(POLKADOT_REPO_DIR)\" ]; then \
				echo \"-- Cloning repository into $(POLKADOT_REPO_DIR)...\"; \
				git clone --depth 1 --branch $$POLKADOT_SDK_RELEASE $(POLKADOT_REPO_URL) $(POLKADOT_REPO_DIR); \
				cd $(POLKADOT_REPO_DIR); \
			else \
				echo \"-- Updating repository...\"; \
				cd $(POLKADOT_REPO_DIR) && git fetch --tags --depth 1 origin && \
				git reset --hard HEAD && \
				git checkout $$POLKADOT_SDK_RELEASE || git checkout -b $$POLKADOT_SDK_RELEASE $$POLKADOT_SDK_RELEASE && \
				git reset --hard $$POLKADOT_SDK_RELEASE; \
			fi && \
			cargo update && \
			cargo build --release --locked -p polkadot-parachain-bin --bin polkadot-parachain && \
			cp $(RESULT_BINARIES_WITH_PATH) /tmp/polkadot_binary/ && \
		 	cd ~ && ./build_apt_package.sh \
		 		$(CURRENT_DATE)-$$POLKADOT_SDK_RELEASE \
		 		$(ARCHITECTURE) \
		 		polkadot-binary \
		 		/tmp/polkadot_binary \
		 		'Polkadot binaries: $(RESULT_BIN_NAMES)' \
		 		'$(POLKADOT_BINARY_DEPENDENCIES)'"; \
		if [ $$? -ne 0 ]; then \
			echo "-- Error: Docker exec failed"; \
			docker stop $$CONTAINER_NAME; \
			exit 1; \
		fi; \
		docker stop $$CONTAINER_NAME; \
	else \
		echo "-- One or more files are missing."; \
	fi

upload_apt_package:
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	POLKADOT_RELEASE_GLOBAL_NUMERIC=$$(grep 'numeric_version:' polkadot-sdk-versions.txt | cut -d ' ' -f 2); \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $$POLKADOT_RELEASE_GLOBAL_NUMERIC-$$SHORT_COMMIT_HASH --package=polkadot-binary --quiet || true ; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/polkadot-binary_$$POLKADOT_RELEASE_GLOBAL_NUMERIC-$${SHORT_COMMIT_HASH}_$(ARCHITECTURE).deb
