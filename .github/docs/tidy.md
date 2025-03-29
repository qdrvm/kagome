
# Tidy Pipeline

This pipeline runs code quality checks using clang-tidy.

## Purpose

The Tidy Pipeline:
- Analyzes Kagome source code with clang-tidy
- Identifies potential issues, bugs, and style violations
- Enforces code quality standards
- Fails the build if significant issues are found

## When It Runs

This pipeline can run:
- As part of the main pipeline (always enabled)
- Manually through workflow_dispatch

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `build_amd64` | Run on AMD64 architecture | `true` |
| `build_arm64` | Run on ARM64 architecture | `false` |
| `builder_latest_tag` | Custom Builder tag | `20250322` |

## Jobs

1. `setup_matrix` - Configures the platform matrix based on parameters
2. `tidy` - Runs clang-tidy on the codebase

## Usage

### Via GitHub UI

1. Go to Actions → Kagome Tidy
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### As a Called Workflow

```yaml
jobs:
  code_quality:
    uses: ./.github/workflows/test-pipeline.yaml
    with:
      build_amd64: 'true'
      build_arm64: 'false'
      builder_latest_tag: '20250322'
    secrets: inherit
```

###  Addressing Issues
When the tidy pipeline fails:

1. Review the logs for specific issues identified by clang-tidy
2. Fix the issues in your code
3. Re-run the pipeline to confirm your fixes resolved the issues
