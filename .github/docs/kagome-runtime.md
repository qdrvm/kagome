
# Kagome Runtime Pipeline

This pipeline builds and publishes the Kagome runtime cache packages.

## Purpose

The Kagome Runtime Pipeline:
- Builds the runtime cache used by Kagome
- Creates Debian packages for the runtime
- Publishes runtime packages to repositories

## When It Runs

This pipeline can run:
- Manually through workflow_dispatch
- When called by other workflows

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `zombie_tester_latest_tag` | Zombie Tester tag | `20250323` |
| `kagome_for_runtime_gen_deb_package_version` | Custom Kagome for runtime gen package version | `20250323-9b65150-Release` |

## Jobs

1. `build_kagome_runtime` - Builds the runtime cache for each architecture
2. `runtime_manifest` - Shows runtime packages information

## Outputs/Artifacts

- Runtime Debian packages in Google Artifact Registry

## Usage

### Via GitHub UI

1. Go to Actions → Kagome Runtime
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### As a Called Workflow

```yaml
jobs:
  build_runtime:
    uses: ./.github/workflows/kagome-runtime.yaml
    with:
      zombie_tester_latest_tag: '20250323'
      kagome_for_runtime_gen_deb_package_version: '20250323-9b65150-Release'
    secrets: inherit
```

### When to Run
This pipeline should be run:

- When there are changes to the runtime code or dependencies
- When you need to update runtime packages for newer versions of Kagome
- When preparing for releases that need fresh runtime cache
