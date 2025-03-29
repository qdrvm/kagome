# Zombie Tests Pipeline

This pipeline runs integration tests for Kagome using the Zombienet framework.

## Purpose

The Zombie Tests Pipeline:
- Runs comprehensive integration tests of Kagome in a network environment
- Tests interoperability with other nodes
- Tests parachains, disputes, consensus, and other features
- Verifies that Kagome works correctly in real-world scenarios

## When It Runs

This pipeline can run:
- As part of the main pipeline (when enabled)
- Manually through workflow_dispatch
- When called by other workflows

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `build_type` | Build type for tests | `Release` |
| `use_cache` | Whether to use cached dependencies | `true` |
| `builder_latest_tag` | Custom Builder tag | `20250322` |
| `zombie_tester_latest_tag` | Custom Zombie Tester tag | `20250323` |
| `runtime_package_version` | Custom runtime package version | `20250323-0.14.1-3fbf6c6` |
| `run_amd64` | Run tests on AMD64 architecture | `true` |
| `run_arm64` | Run tests on ARM64 architecture | `true` |
| `skip_build` | Skip building packages (when called from main pipeline) | `false` |

## Jobs

1. `check_package_exists` - Checks if required packages already exist
2. `kagome_dev_docker_build` - Builds Kagome if needed
3. `setup_matrix` - Configures the test matrix
4. `zombie_tests` - Runs the zombie tests

## Test Scenarios

The zombie tests include multiple scenarios:
- PVF preparation & execution time
- BEEFY voting and finality, MMR proofs
- Dispute handling and validation
- Approval voting coalescing
- Block production rate testing
- Systematic chunk recovery
- Warp sync from Polkadot node
- And more...

## Outputs/Artifacts

- Test logs as GitHub artifacts
- Detailed test output in the job logs

## Usage

### Via GitHub UI

1. Go to Actions → Kagome Runtime
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### As a Called Workflow

```yaml
jobs:
  integration_tests:
    uses: ./.github/workflows/zombie-tests.yaml
    with:
      build_type: 'Release'
      use_cache: 'true'
      builder_latest_tag: '20250322'
      zombie_tester_latest_tag: '20250323'
      runtime_package_version: '20250323-0.14.1-3fbf6c6'
      run_amd64: 'true'
      run_arm64: 'false'
      skip_build: 'true'
    secrets: inherit
```

### Debugging Tests
When tests fail:

1. Download test logs from the artifacts
2. Review the logs for specific issues
3. Use the same Docker images locally to reproduce the issue
4. Fix the issues and re-run the tests
