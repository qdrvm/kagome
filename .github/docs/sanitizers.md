
# Sanitizers Pipeline

This pipeline builds Kagome with various sanitizers to detect runtime issues.

## Purpose

The Sanitizers Pipeline:
- Builds Kagome with Thread Sanitizer (TSAN) to detect thread safety issues
- Builds Kagome with Address Sanitizer (ASAN) to detect memory errors
- Builds Kagome with Undefined Behavior Sanitizer (UBSAN) to detect undefined behavior
- Runs tests with sanitizer instrumentation
- Produces Docker images with sanitizer builds

## When It Runs

This pipeline can run:
- As part of the main pipeline (always enabled)
- Manually through workflow_dispatch

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `build_tsan` | Build with Thread Sanitizer | `true` |
| `build_asanubsan` | Build with Address and UB Sanitizers | `true` |
| `build_amd64` | Build for AMD64 architecture | `true` |
| `build_arm64` | Build for ARM64 architecture | `false` |
| `use_cache` | Whether to use cached dependencies | `true` |
| `cache_upload_allowed` | Whether to upload cache after build | `false` |
| `builder_latest_tag` | Custom Builder tag | `20250322` |
| `sanitizer_image_tag` | Custom Sanitizer Image tag | `latest` |

## Jobs

1. `setup_matrix` - Configures the sanitizer matrix based on parameters
2. `build_sanitizers` - Builds Kagome with sanitizer instrumentation
3. `docker_manifest` - Creates and pushes multi-architecture Docker manifests for sanitizer builds

## Outputs/Artifacts

- Sanitizer Debian packages in Google Artifact Registry
- Sanitizer Docker images in Google Container Registry

## Usage

### Via GitHub UI

1. Go to Actions → Kagome Sanitizers
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### As a Called Workflow

```yaml
jobs:
  sanitizers_job:
    uses: ./.github/workflows/sanitizers.yaml
    with:
      build_tsan: 'true'
      build_asanubsan: 'true'
      build_amd64: 'true'
      build_arm64: 'false'
      use_cache: 'true'
      cache_upload_allowed: 'false'
      builder_latest_tag: '20250322'
    secrets: inherit
```

## Debugging with Sanitizers
When sanitizers detect issues:

1. Review the logs for specific issues identified by the sanitizers
2. Use the sanitizer Docker images for local debugging
3. Fix the issues and re-run the sanitizers to verify your fixes
