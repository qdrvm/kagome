# Build Pipeline

This pipeline handles building Kagome binaries, Docker images, and Debian packages.

## Purpose

The Build Pipeline:
- Builds Kagome binaries with various configurations (Debug, Release, RelWithDebInfo)
- Packages binaries as Debian packages
- Creates Docker images for Kagome
- Pushes artifacts to repositories
- Creates multi-architecture manifests

## When It Runs

This pipeline can run:
- As part of the main pipeline
- Manually through workflow_dispatch
- When called by other workflows (like zombie-tests)

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `build_amd64` | Build for AMD64 architecture | `true` |
| `build_arm64` | Build for ARM64 architecture | `true` |
| `build_debug` | Build Debug configuration | `true` |
| `build_release` | Build Release configuration | `true` |
| `build_relwithdebinfo` | Build RelWithDebInfo configuration | `true` |
| `use_cache` | Whether to use cached dependencies | `true` |
| `cache_upload_allowed` | Whether to upload cache after build | `false` |
| `builder_latest_tag` | Custom Builder tag | `20250322` |

## Jobs

1. `setup_matrix` - Configures the build matrix based on parameters
2. `build` - Builds binaries and packages for each platform/build type combination
3. `docker_manifest` - Creates and pushes multi-architecture Docker manifests

## Outputs/Artifacts

- Debian packages in Google Artifact Registry
- Docker images in Google Container Registry
- Docker images in Docker Hub (for Release builds only)

## Usage

### Via GitHub UI

1. Go to Actions → Kagome Build
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### As a Called Workflow

```yaml
jobs:
  build_job:
    uses: ./.github/workflows/build.yaml
    with:
      build_amd64: 'true'
      build_arm64: 'false'
      build_debug: true
      build_release: true
      build_relwithdebinfo: true
      use_cache: 'true'
      cache_upload_allowed: 'false'
      builder_latest_tag: '20250322'
    secrets: inherit
```

## Best practices

- For quicker builds, only enable the architectures and build types you need
- Set `use_cache=true` to speed up builds with caching
- Only set `cache_upload_allowed=true` when you've made significant dependency changes
