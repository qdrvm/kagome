kagome_dev_docker_build_tidy:
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
		-v $(CACHE_DIR)/.cargo/git:/root/.cargo/git \
		-v $(CACHE_DIR)/.cargo/registry:/root/.cargo/registry \
		-v $(CACHE_DIR)/.hunter:/root/.hunter \
		-v $(CACHE_DIR)/.cache/ccache:/root/.cache/ccache \
		$(DOCKERHUB_BUILDER_PATH):$(BUILDER_IMAGE_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"clang --version && \
		cd /opt/kagome && \
		git config --global --add safe.directory /opt/kagome && \
		git config --global --add safe.directory /root/.hunter/_Base/Cache/meta && \
		source /venv/bin/activate && \
		git submodule update --init && \
		echo \"Building in $$(pwd)\" && \
		cmake . -B\"$(BUILD_DIR)\" -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=\"$(BUILD_TYPE)\" -DBACKWARD=OFF -DWERROR=$(WERROR) && \
		cmake --build \"$(BUILD_DIR)\" --target generated -- -j$(BUILD_THREADS) && \
		cd /opt/kagome/ && export CI='$(CI)' && ./housekeeping/clang-tidy-diff.sh \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME
