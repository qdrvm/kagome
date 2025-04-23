
# Kagome Builder Pipeline

This pipeline builds the Docker images used for building Kagome.

## Purpose

The Kagome Builder Pipeline:
- Creates Docker images with all dependencies needed to build Kagome
- Builds images for multiple architectures (AMD64, ARM64)
- Pushes images to Google Container Registry and DockerHub
- Creates multi-architecture manifests

## When It Runs

This pipeline can run:
- Manually through workflow_dispatch
- Scheduled runs can be enabled (commented out by default)

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `builder_latest_tag` | Kagome Builder tag (latest or date like "20251201") | `latest` |

## Jobs

1. `build_kagome_builder` - Builds the builder images for each architecture
2. `docker_manifest` - Creates and pushes multi-architecture Docker manifests

## Outputs/Artifacts

- Builder Docker images in Google Container Registry
- Builder Docker images in DockerHub

## Usage

### Via GitHub UI

1. Go to Actions → Kagome Builder | Ubuntu (24.04_LTS)
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### When to Update

The builder images should be updated:
- When dependencies need to be updated
- When newer Ubuntu/compiler versions should be used
- When new tools need to be included in the build environment
- Periodically to include security updates
