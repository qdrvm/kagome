#RUST_IMAGE=rust:1.79-slim-bookworm

RUST_VERSION ?= 1.79.0
# bookworm (Debian 12)
MINIDEB_IMAGE ?= bitnami/minideb@sha256:6cc3baf349947d587a9cd4971e81ff3ffc0d17382f2b5b6de63d6542bff10c16

PLATFORM ?= linux/amd64
ARCHITECTURE ?= x86_64

DOCKER_REGISTRY_PATH ?=

MINIDEB_HASH := $(subst bitnami/minideb@sha256:,,$(MINIDEB_IMAGE))
MINIDEB_SHORT_HASH = $(shell echo $(MINIDEB_HASH) | cut -c1-7)
MINIDEB_TAG = $(MINIDEB_SHORT_HASH)_rust-$(RUST_VERSION)

export DOCKER_BUILDKIT=1
# BUILDKIT_PROGRESS - auto, plain, tty, rawjson
export BUILDKIT_PROGRESS=auto

kagome_builder_deb:
	docker build --platform $(PLATFORM) \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(MINIDEB_TAG) \
    		-t $(DOCKER_REGISTRY_PATH)kagome_builder_deb:latest \
    		-f kagome_builder_deb.Dockerfile \
    		--build-arg RUST_VERSION=$(RUST_VERSION) \
    		--build-arg BASE_IMAGE=$(MINIDEB_IMAGE) \
    		--build-arg ARCHITECTURE=$(ARCHITECTURE) .

kagome_builder_deb_push:
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(MINIDEB_TAG) ; \
	docker push $(DOCKER_REGISTRY_PATH)kagome_builder_deb:latest ; \
