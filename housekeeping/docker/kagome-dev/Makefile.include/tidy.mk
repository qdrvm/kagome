.PHONY: kagome_dev_docker_build_tidy tidy_prepare tidy_clean tidy_run tidy_exec

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
	docker stop $(CONTAINER_NAME) > /dev/null 2>&1 || true
	docker rm -f $(CONTAINER_NAME) > /dev/null 2>&1 || true

# Start tidy container
tidy_run:
	@echo "-- Starting tidy container..."
	$(MAKE) tidy_clean
	mkdir -p \
		$(CACHE_DIR)/.cargo \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	echo "Build type: $(BUILD_TYPE)"; \
	echo "Architecture: $(ARCHITECTURE) (Platform: $(PLATFORM))"; \
	docker run -d --name $(CONTAINER_NAME) \
		--platform $(PLATFORM) \
		-e SHORT_COMMIT_HASH=$(SHORT_COMMIT_HASH) \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e PLATFORM=$(PLATFORM) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-e BUILD_DIR="$(DOCKER_BUILD_DIR_NAME)" \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e MOUNTED_DIRS="/home/builder /opt/kagome" \
		-v $(WORKING_DIR):/opt/kagome \
		-v $(CACHE_DIR)/.cargo/registry:$(IN_DOCKER_HOME)/.cargo/registry \
		-v $(CACHE_DIR)/.cargo/git:$(IN_DOCKER_HOME)/.cargo/git \
		-v $(CACHE_DIR)/.hunter:$(IN_DOCKER_HOME)/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:$(IN_DOCKER_HOME)/.cache/ccache \
		$(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
		tail -f /dev/null; \
	until [ "$$(docker inspect --format='{{.State.Health.Status}}' $(CONTAINER_NAME))" = "healthy" ]; do \
	  echo "Health is: \"$$(docker inspect --format='{{.State.Health.Status}}' $(CONTAINER_NAME))\"" ; \
	  sleep 5 ; \
	done


# Execute tidy check in container
tidy_exec:
	@echo "-- Running clang-tidy check..."
	DOCKER_EXEC_RESULT=0; \
	docker exec -t $(CONTAINER_NAME) gosu $(USER_ID):$(GROUP_ID) /bin/bash -c \
		"cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory $(IN_DOCKER_HOME)/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"-- Recent commits:\" && git log --oneline -n 5 && \
		echo \"-- Running clang-tidy check...\" && \
		cmake . -B"$(DOCKER_BUILD_DIR_NAME)" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build $(DOCKER_BUILD_DIR_NAME) --target generated -- -j$(BUILD_THREADS) && \
		cd /opt/kagome/ && export CI='$(CI)' && \
		./housekeeping/clang-tidy-diff.sh -b $(DOCKER_BUILD_DIR_NAME) \
		" || DOCKER_EXEC_RESULT=$$?; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	echo "-- Clang-tidy check completed successfully!"
