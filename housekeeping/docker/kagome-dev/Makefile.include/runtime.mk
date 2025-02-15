runtime_cache:
	CONTAINER_NAME=kagome_dev_runtime_cache_$$(openssl rand -hex 6); \
	RUNTIME_VERSION=$$(date +'%y.%m.%d')-$$(python3 get_wasmedge_version.py); \
	DOCKER_EXEC_RESULT=0 ; \
	echo "Runtime version: $$RUNTIME_VERSION"; \
	docker run -d --name $$CONTAINER_NAME \
		--platform $(PLATFORM) \
		--entrypoint "/bin/bash" \
		-e RUNTIME_VERSION=$$RUNTIME_VERSION \
		-e PACKAGE_ARCHITECTURE=$(PACKAGE_ARCHITECTURE) \
		-e GOOGLE_APPLICATION_CREDENTIALS=/root/.gcp/google_creds.json \
		-v $$(pwd)/../../../../kagome:/opt/kagome \
		-v $(GOOGLE_APPLICATION_CREDENTIALS):/root/.gcp/google_creds.json \
		$(DOCKER_REGISTRY_PATH)zombie_tester:$(TESTER_LATEST_TAG) \
		-c "tail -f /dev/null"; \
	docker exec -t $$CONTAINER_NAME /bin/bash -c \
		"cd /opt/kagome/zombienet && \
		sed -i '1s/^#//' /etc/apt/sources.list.d/kagome.list && \
		install_packages kagome-dev && \
		./precompile.sh && \
		cd /opt/kagome/housekeeping/docker/kagome-dev && \
		mkdir -p /tmp/kagome_runtime && \
		cp -r /tmp/kagome/runtimes-cache/* /tmp/kagome_runtime && \
		./build_apt_package.sh \
			\"$${RUNTIME_VERSION}\" \
			$(PACKAGE_ARCHITECTURE) \
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
	docker stop $$CONTAINER_NAME

upload_apt_package_runtime:
	RUNTIME_VERSION=$$(date +'%y.%m.%d')-$$(python3 get_wasmedge_version.py); \
	echo "Runtime version: $$RUNTIME_VERSION"; \
	gcloud config set artifacts/repository $(ARTIFACTS_REPO); \
	gcloud config set artifacts/location $(REGION); \
	RUNTIME_PACKAGE_EXIST=$$(gcloud artifacts versions list --package=kagome-dev-runtime --format=json | jq -e ".[] | select(.name | endswith(\"${RUNTIME_VERSION}\"))" > /dev/null && echo "True" || echo "False"); \
	if [ "$$RUNTIME_PACKAGE_EXIST" = "True" ]; then \
		echo "Package with version $$RUNTIME_VERSION already exists"; \
		gcloud artifacts versions delete $$RUNTIME_VERSION --package=kagome-dev-runtime --quiet; \
	fi; \
	gcloud artifacts apt upload $(ARTIFACTS_REPO) --source=./pkg/kagome-dev-runtime_$${RUNTIME_VERSION}_$(PACKAGE_ARCHITECTURE).deb
