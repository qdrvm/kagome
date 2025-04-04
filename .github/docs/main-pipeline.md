# Main Pipeline

This is the primary CI pipeline that orchestrates the entire build and test process for the Kagome project.

## Purpose

The Main Pipeline coordinates the execution of all other pipelines:
- Builds Kagome on various platforms and architectures
- Runs code quality checks (tidy)
- Runs sanitizers for detecting issues
- Runs zombie tests for integration testing
- Builds MacOS versions

## When It Runs

This pipeline runs:
- On every push to master
- On every tag creation
- On every pull request
- Manually through workflow_dispatch

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `build_type` | Build type for zombie tests | `Release` |
| `use_cache` | Whether to use cached dependencies | `true` |
| `cache_upload_allowed` | Whether to upload cache after build | `false` |
| `run_amd64` | Run on AMD64 architecture | `true` |
| `run_arm64` | Run on ARM64 architecture | `false` |
| `run_zombie_tests` | Run zombie tests | `true` |
| `builder_latest_tag` | Custom Builder tag | `20250322` |
| `zombie_tester_latest_tag` | Custom Zombie Tester tag | `20250323` |
| `runtime_package_version` | Custom runtime package version | `20250323-0.14.1-3fbf6c6` |

## Jobs

1. `diagnostic` - Displays all configuration parameters and version information
2. `macos` - Builds Kagome on macOS with different configurations
3. `tidy` - Runs code quality checks (always enabled)
4. `sanitizers` - Builds with various sanitizers (always enabled)
5. `build` - Builds Kagome binaries and Docker images
6. `zombie_tests` - Runs integration tests using Zombienet

## Usage

### Via GitHub UI

1. Go to Actions → Main Build Pipeline (New)
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### For Development

For development and testing, it's recommended to:
1. Set `run_amd64=true` and `run_arm64=false` to build only for your development architecture
2. Set `run_zombie_tests=false` during development to skip time-consuming integration tests
3. Use `use_cache=true` to speed up builds

### For Release

For release builds:
1. Run on both architectures: `run_amd64=true` and `run_arm64=true`
2. Enable all tests: `run_zombie_tests=true`
3. Set `cache_upload_allowed=true` to update caches for future builds
