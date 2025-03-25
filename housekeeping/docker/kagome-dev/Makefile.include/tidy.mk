# Tidy container name
TIDY_CONTAINER_NAME := kagome_tidy_$(ARCHITECTURE)_$(shell echo $(PLATFORM) | sha256sum | cut -c1-8)

# Main tidy target
kagome_dev_docker_build_tidy:
	$(MAKE) tidy_prepare
	$(MAKE) tidy_run
	$(MAKE) tidy_exec || { $(MAKE) tidy_clean; exit 1; }
	$(MAKE) tidy_clean

# Prepare for tidy check
tidy_prepare:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache

# Clean any existing tidy container
tidy_clean:
	@echo "-- Cleaning tidy container..."
	docker stop $(TIDY_CONTAINER_NAME) > /dev/null 2>&1 || true
	docker rm -f $(TIDY_CONTAINER_NAME) > /dev/null 2>&1 || true

# Start tidy container
tidy_run:
	@echo "-- Starting tidy container..."
	$(MAKE) tidy_clean
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	echo "Build type: $(BUILD_TYPE)"; \
	echo "Architecture: $(ARCHITECTURE) (Platform: $(PLATFORM))"; \
	docker run -d --name $(TIDY_CONTAINER_NAME) \
		--platform $(PLATFORM) \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e PLATFORM=$(PLATFORM) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-e BUILD_DIR="build_docker_$(ARCHITECTURE)_$(BUILD_TYPE)" \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e MOUNTED_DIRS="/opt/kagome" \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
		tail -f /dev/null; \
	MAX_TRIES=30; \
	TRIES=0; \
	while [ $$TRIES -lt $$MAX_TRIES ]; do \
		HEALTH_STATUS="$$(docker inspect --format='{{.State.Health.Status}}' $(TIDY_CONTAINER_NAME) 2>/dev/null || echo 'starting')"; \
		echo "Health status: $$HEALTH_STATUS (try $$TRIES/$$MAX_TRIES)"; \
		if [ "$$HEALTH_STATUS" = "healthy" ]; then \
			echo "Container is healthy"; \
			break; \
		fi; \
		if [ $$TRIES -eq $$((MAX_TRIES-1)) ]; then \
			echo "Container failed to become healthy after $$MAX_TRIES attempts"; \
			docker logs $(TIDY_CONTAINER_NAME); \
			docker stop $(TIDY_CONTAINER_NAME); \
			docker rm -f $(TIDY_CONTAINER_NAME); \
			exit 1; \
		fi; \
		TRIES=$$((TRIES+1)); \
		sleep 5; \
	done

# Execute tidy check in container
tidy_exec:
	@echo "-- Running clang-tidy check..."
	DOCKER_EXEC_RESULT=0; \
	BUILD_DIR_NAME="build_docker_$(ARCHITECTURE)_$(BUILD_TYPE)"; \
	docker exec -t $(TIDY_CONTAINER_NAME) gosu $(USER_ID):$(GROUP_ID) /bin/bash -c \
		"cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in \$$(pwd) for architecture \$$(arch)\" && \
		mkdir -p \"\$$BUILD_DIR_NAME\" && \
		cmake . -B\"\$$BUILD_DIR_NAME\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"\$$BUILD_DIR_NAME\" --target generated -- -j$(BUILD_THREADS) && \
		cd /opt/kagome/ && export CI='$(CI)' && \
		./housekeeping/clang-tidy-diff.sh -b \"\$$BUILD_DIR_NAME\" \
		" || DOCKER_EXEC_RESULT=$$?; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	echo "-- Clang-tidy check completed successfully!"
