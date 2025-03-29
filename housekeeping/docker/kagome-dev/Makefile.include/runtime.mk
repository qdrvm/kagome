define PACKAGE_VERSION
$(if $(KAGOME_FOR_RUNTIME_GEN_DEB_PACKAGE_VERSION_NO_ARCH),=$(KAGOME_FOR_RUNTIME_GEN_DEB_PACKAGE_VERSION_NO_ARCH)-$(ARCHITECTURE))
endef

runtime_cache:
	CONTAINER_NAME=kagome_dev_runtime_cache_$$(openssl rand -hex 6); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Runtime version: $(RUNTIME_VERSION)"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		-e USER_ID=$(USER_ID) \
		-e GROUP_ID=$(GROUP_ID) \
		-e RUNTIME_VERSION=$(RUNTIME_VERSION) \
		-e ARCHITECTURE=$(ARCHITECTURE) \
		-e PACKAGE_VERSION="$(PACKAGE_VERSION)" \
		-e GOOGLE_APPLICATION_CREDENTIALS=/root/.gcp/google_creds.json \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		$(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG) \
		tail -f /dev/null ; \
	until [ "$$(docker inspect --format='{{.State.Health.Status}}' $$CONTAINER_NAME)" = "healthy" ]; do \
	  echo "Health is: \"$$(docker inspect --format='{{.State.Health.Status}}' $$CONTAINER_NAME)\"" ; \
	  sleep 5 ; \
	done ; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome/zombienet && \
		sed -i '1s/^#//' /etc/apt/sources.list.d/kagome.list && \
		install_packages kagome-dev$(PACKAGE_VERSION) && \
		./precompile.sh && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		mkdir -p /tmp/kagome_runtime && \
		cp -r /tmp/kagome/runtimes-cache/* /tmp/kagome_runtime && \
		./build_apt_package.sh \
			\"$(RUNTIME_VERSION)\" \
			$(ARCHITECTURE) \
			kagome-dev-runtime \
			/tmp/kagome_runtime \
			'Kagome Runtime Dev Ubuntu Package' \
			'kagome-dev' \
			 /tmp/kagome/runtimes-cache/ ; \
		" || DOCKER_EXEC_RESULT=$$? ; \
	if [ $$DOCKER_EXEC_RESULT -ne 0 ]; then \
		echo "Error: Docker exec failed with return code $$DOCKER_EXEC_RESULT"; \
		docker stop $$CONTAINER_NAME; \
		exit $$DOCKER_EXEC_RESULT; \
	fi; \
	docker stop $$CONTAINER_NAME ; \
	docker rm $$CONTAINER_NAME

upload_apt_package_runtime:
	echo "Runtime version: $(RUNTIME_VERSION)"; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	RUNTIME_PACKAGE_EXIST=$$(gcloud artifacts versions list --package=kagome-dev-runtime --format=json | jq -e ".[] | select(.name | endswith(\"${RUNTIME_VERSION}\"))" > /dev/null && echo "True" || echo "False"); \
	if [ "$$RUNTIME_PACKAGE_EXIST" = "True" ]; then \
		echo "Package with version $(RUNTIME_VERSION) already exists"; \
		gcloud artifacts versions delete $(RUNTIME_VERSION) --package=kagome-dev-runtime --quiet; \
	fi; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev-runtime_$(RUNTIME_VERSION)_$(ARCHITECTURE).deb

.PHONY: runtime_cache upload_apt_package_runtime
