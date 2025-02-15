kagome_dev_docker_build:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	CONTAINER_NAME=kagome_dev_build_$$(openssl rand -hex 6); \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Build type: $(BUILD_TYPE)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $$(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"$(BUILD_DIR)\" --target kagome -- -j$(BUILD_THREADS) && \
		mkdir -p /tmp/kagome && \
		cp /opt/kagome/$(BUILD_DIR)/node/kagome /tmp/kagome/kagome && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		./build_apt_package.sh \
			\"$$(date +'%y.%m.%d')-$${SHORT_COMMIT_HASH}-$(BUILD_TYPE)\" \
			$(PACKAGE_ARCHITECTURE) \
			kagome-dev \
			/tmp/kagome \
			'Kagome Dev Ubuntu Package' \
			'$(DEPENDENCIES)' && \
		if [ "$(IS_MAIN_OR_TAG)" = "true" ] && [ "$(GIT_REF_NAME)" != "master" ] && [ "$(BUILD_TYPE)" = "Release" ]; then \
			./build_apt_package.sh \
				\"$(KAGOME_SANITIZED_VERSION)-$(BUILD_TYPE)\" \
				$(PACKAGE_ARCHITECTURE) \
				kagome \
				/tmp/kagome \
				'Kagome Ubuntu Package' \
				'$(DEPENDENCIES)' ; \
		fi; \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME

upload_apt_package:
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev_$$(date +'%y.%m.%d')-$${SHORT_COMMIT_HASH}-$(BUILD_TYPE)_$(PACKAGE_ARCHITECTURE).deb ; \
	if [ "$(IS_MAIN_OR_TAG)" = "true" ] && [ "$(GIT_REF_NAME)" != "master" ] && [ "$(BUILD_TYPE)" = "Release" ]; then \
		gcloud artifacts apt upload $(PUBLIC_ARTIFACTS_REPO) --source=./pkg/kagome_$(KAGOME_SANITIZED_VERSION)-$(BUILD_TYPE)_$(PACKAGE_ARCHITECTURE).deb ; \
	fi;
