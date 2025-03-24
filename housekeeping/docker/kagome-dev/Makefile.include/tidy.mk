kagome_dev_docker_build_tidy:
	$(MAKE) get_versions
	mkdir -p \
		$(CACHE_DIR)/.cargo/git \
		$(CACHE_DIR)/.cargo/registry \
		$(CACHE_DIR)/.hunter \
		$(CACHE_DIR)/.cache/ccache  ; \
	SHORT_COMMIT_HASH=$$(grep 'short_commit_hash:' commit_hash.txt | cut -d ' ' -f 2); \
	CONTAINER_NAME=kagome_dev_tidy_$(ARCHITECTURE)_$$(echo $(PLATFORM) | sha256sum | cut -c1-8); \
	echo "Using container name: $$CONTAINER_NAME"; \
	echo "Removing any existing container with the same name..."; \
	docker rm -f $$CONTAINER_NAME > /dev/null 2>&1 || true; \
	DOCKER_EXEC_RESULT=0 ; \
	trap 'echo "Cleaning up container $$CONTAINER_NAME"; docker rm -f $$CONTAINER_NAME > /dev/null 2>&1 || true' EXIT INT TERM; \
	echo "Build type: $(BUILD_TYPE)"; \
	echo "Architecture: $(ARCHITECTURE) (Platform: $(PLATFORM))"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e SHORT_COMMIT_HASH=$$SHORT_COMMIT_HASH \
		-e BUILD_TYPE=$(BUILD_TYPE) \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e PLATFORM=$(PLATFORM) \
		-e GITHUB_HUNTER_USERNAME=$(GITHUB_HUNTER_USERNAME) \
		-e GITHUB_HUNTER_TOKEN=$(GITHUB_HUNTER_TOKEN) \
		-e CTEST_OUTPUT_ON_FAILURE=$(CTEST_OUTPUT_ON_FAILURE) \
		-e BUILD_DIR=$(DOCKER_BUILD_DIR_NAME) \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e MOUNTED_DIRS="/opt/kagome" \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	echo "Waiting for container health check..."; \
	until [ "$$(docker inspect --format='{{.State.Health.Status}}' $$CONTAINER_NAME 2>/dev/null)" = "healthy" ]; do \
		echo "Health status: $$(docker inspect --format='{{.State.Health.Status}}' $$CONTAINER_NAME 2>/dev/null || echo 'checking')" ; \
		sleep 5 ; \
	done; \
	echo "Container is healthy, proceeding with tidy check"; \
	docker exec -t $$CONTAINER_NAME gosu $(USER_ID):$(GROUP_ID) /bin/bash -c \
		"echo \"Running on architecture: \$$(arch)\" && \
		clang --version && \
		cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in \$$(pwd) for architecture \$$(arch)\" && \
		cmake . -B\"$(DOCKER_BUILD_DIR_NAME)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"$(DOCKER_BUILD_DIR_NAME)\" --target generated -- -j$(BUILD_THREADS) && \
		cd /opt/kagome/ && export CI='$(CI)' && ./housekeeping/clang-tidy-diff.sh \
		" || DOCKER_EXEC_RESULT=$$? ; \
	docker stop $$CONTAINER_NAME || true; \
	docker rm $$CONTAINER_NAME || true; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		exit $$DOCKER_EXEC_RESULT; \
	fi
