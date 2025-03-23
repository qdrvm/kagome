# Default sanitizer configuration
BUILD_TYPE_SANITIZERS ?= RelWithDebInfo

# Sanitizer-specific build directory
define sanitizer_build_dir
$(WORKING_DIR)/build_docker_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZERS)_$(1)
endef

# Common function for sanitizer container creation
define sanitizer_container_create
	echo "-- Creating container for $(1) build..."
	mkdir -p \
		$(call sanitizer_build_dir,$(1))/pkg \
		$(call sanitizer_build_dir,$(1))/kagome_binary \
		$(call sanitizer_cache_dir,$(1))/.cargo \
		$(call sanitizer_cache_dir,$(1))/.hunter \
		$(call sanitizer_cache_dir,$(1))/.cache/ccache; \
	CONTAINER_NAME=kagome_$(1)_build_$$(openssl rand -hex 6); \
	echo "Build type: $(BUILD_TYPE_SANITIZERS) with $(1)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		-e SHORT_COMMIT_HASH=$(SHORT_COMMIT_HASH) \
		-e BUILD_TYPE=$(BUILD_TYPE_SANITIZERS) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-e UBSAN_OPTIONS=$(UBSAN_OPTIONS) \
		-e MOUNTED_DIRS="/home/builder /opt/kagome" \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):$(IN_DOCKER_HOME)/.gcp/google_creds.json \
		-v ./build_apt_package.sh:$(IN_DOCKER_HOME)/build_apt_package.sh \
		-v $(WORKING_DIR):/opt/kagome \
		-v $(call sanitizer_build_dir,$(1))/pkg:$(IN_DOCKER_HOME)/pkg \
		-v $(call sanitizer_build_dir,$(1))/kagome_binary:$(IN_DOCKER_HOME)/kagome_binary \
		-v $(call sanitizer_cache_dir,$(1))/.cargo/registry:$(IN_DOCKER_HOME)/.cargo/registry \
		-v $(call sanitizer_cache_dir,$(1))/.cargo/git:$(IN_DOCKER_HOME)/.cargo/git \
		-v $(call sanitizer_cache_dir,$(1))/.hunter:$(IN_DOCKER_HOME)/.hunter \
		-v $(call sanitizer_cache_dir,$(1))/.cache/ccache:$(IN_DOCKER_HOME)/.cache/ccache \
		$(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_IMAGE_TAG) \
		tail -f /dev/null; \
	until [ "$$(docker inspect --format='{{.State.Health.Status}}' $$CONTAINER_NAME)" = "healthy" ]; do \
		echo "Health is: \"$$(docker inspect --format='{{.State.Health.Status}}' $$CONTAINER_NAME)\""; \
		sleep 5; \
	done; \
	echo "Container $$CONTAINER_NAME is healthy"; \
	echo $$CONTAINER_NAME > $(1)_container.id
endef

# Common function for sanitizer build execution
define sanitizer_build_exec
	echo "-- Building Kagome with $(1)..."
	CONTAINER_NAME=$$(cat $(1)_container.id); \
	DOCKER_EXEC_RESULT=0; \
	SANITIZER_BUILD_DIR=build_docker_$(ARCHITECTURE)_$(BUILD_TYPE_SANITIZERS)_$(1); \
	docker exec -t $$CONTAINER_NAME gosu $(USER_ID):$(GROUP_ID) /bin/bash -c \
		"cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory $(IN_DOCKER_HOME)/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building kagome with $(1) in \$$(pwd)\" && \
		time cmake . -B\"$$SANITIZER_BUILD_DIR\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE_SANITIZERS)\" \
			-DBACKWARD=OFF -DWERROR=$(WERROR) $($(shell echo $(1) | tr a-z A-Z)_PARAMS) && \
		time cmake --build \"$$SANITIZER_BUILD_DIR\" -- -j$(BUILD_THREADS) && \
		cp /opt/kagome/$$SANITIZER_BUILD_DIR/node/kagome $(IN_DOCKER_HOME)/kagome_binary/kagome && \
		echo \"Running tests with $(1)...\" && \
		cmake --build \"$$SANITIZER_BUILD_DIR\" --target test && \
		cd $(IN_DOCKER_HOME) && ./build_apt_package.sh \
			$(KAGOME_DEB_PACKAGE_VERSION)-$(1) \
			$(ARCHITECTURE) \
			kagome-$(1) \
			$(IN_DOCKER_HOME)/kagome_binary \
			'Kagome $(BUILD_TYPE_SANITIZERS) with $(1) Ubuntu Package' \
			'$(DEPENDENCIES)' \
		" || DOCKER_EXEC_RESULT=$$?; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		docker rm $$CONTAINER_NAME; \
		rm -f $(1)_container.id; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME; \
	docker rm $$CONTAINER_NAME; \
	rm -f $(1)_container.id; \
	echo "-- $(1) build and package creation completed."
endef

# Common function to clean container
define sanitizer_clean_container
	@echo "-- Cleaning $(1) container..."; \
	if [ -f $(1)_container.id ]; then \
		CONTAINER_NAME=$$(cat $(1)_container.id); \
		docker stop $$CONTAINER_NAME || true; \
		docker rm $$CONTAINER_NAME || true; \
		rm -f $(1)_container.id; \
	fi
endef

# Common function for uploading sanitizer package
define sanitizer_upload_package
	@echo "-- Uploading $(1) package to Google Cloud Storage..."; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	gcloud artifacts versions delete $(KAGOME_DEB_PACKAGE_VERSION)-$(1) --package=kagome-$(1) --quiet || true; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=$(call sanitizer_build_dir,$(1))/pkg/kagome-$(1)_$(KAGOME_DEB_PACKAGE_VERSION)-$(1)_$(ARCHITECTURE).deb; \
	echo "-- $(1) package uploaded."
endef

# Build targets for each sanitizer (with cache support)
kagome_docker_build_asan:
	$(call sanitizer_container_create,asan)
	$(call sanitizer_build_exec,asan)

kagome_docker_build_tsan:
	$(call sanitizer_container_create,tsan)
	$(call sanitizer_build_exec,tsan)

kagome_docker_build_ubsan:
	$(call sanitizer_container_create,ubsan)
	$(call sanitizer_build_exec,ubsan)

kagome_docker_build_asanubsan:
	$(call sanitizer_container_create,asanubsan)
	$(call sanitizer_build_exec,asanubsan)

# Clean container targets
kagome_clean_asan:
	$(call sanitizer_clean_container,asan)

kagome_clean_tsan:
	$(call sanitizer_clean_container,tsan)

kagome_clean_ubsan:
	$(call sanitizer_clean_container,ubsan)

kagome_clean_asanubsan:
	$(call sanitizer_clean_container,asanubsan)

# Package upload targets
kagome_upload_asan_package:
	$(call sanitizer_upload_package,asan)

kagome_upload_tsan_package:
	$(call sanitizer_upload_package,tsan)

kagome_upload_ubsan_package:
	$(call sanitizer_upload_package,ubsan)

kagome_upload_asanubsan_package:
	$(call sanitizer_upload_package,asanubsan)

# Sanitizer builds with test but without packaging
kagome_test_asan:
	@echo "-- Building and testing with Address Sanitizer..."
	$(MAKE) kagome_docker_build_asan

kagome_test_tsan:
	@echo "-- Building and testing with Thread Sanitizer..."
	$(MAKE) kagome_docker_build_tsan

kagome_test_ubsan:
	@echo "-- Building and testing with Undefined Behavior Sanitizer..."
	$(MAKE) kagome_docker_build_ubsan

kagome_test_asanubsan:
	@echo "-- Building and testing with Address and Undefined Behavior Sanitizers..."
	$(MAKE) kagome_docker_build_asanubsan

# Run all sanitizer tests
kagome_test_all_sanitizers: kagome_test_asan kagome_test_tsan kagome_test_ubsan kagome_test_asanubsan
	@echo "-- All sanitizer tests completed."

# Legacy sanitizer target for backwards compatibility
kagome_dev_docker_build_sanitizers: kagome_test_asan
	@echo "-- Legacy sanitizer build completed (ASAN only)."
