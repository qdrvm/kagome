# Zombie Builder Pipeline

This pipeline builds Docker images and binaries for Polkadot used in Zombienet tests.

## Purpose

The Zombie Builder Pipeline:
- Creates Docker images for building Polkadot and related binaries
- Builds Polkadot binaries for testing
- Packages binaries as Debian packages
- Supports multiple architectures (AMD64, ARM64)

## When It Runs

This pipeline can run:
- Manually through workflow_dispatch
- Scheduled runs can be enabled (commented out by default)

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `polkadot_builder` | Build the polkadot builder image | `false` |
| `polkadot_binaries` | Build the polkadot binaries | `true` |
| `polkadot_builder_tag` | Polkadot Builder tag | `latest` |
| `polkadot_sdk_tag` | Custom Polkadot SDK tag | `` |
| `polkadot_sdk_repo` | Custom Polkadot SDK repo | `https://github.com/paritytech/polkadot-sdk.git` |

## Jobs

1. `build_polkadot_builder` - Builds the Polkadot builder images
2. `docker_manifest` - Creates multi-architecture manifests for builder images
3. `building_binaries` - Builds Polkadot binaries and creates Debian packages

## Outputs/Artifacts

- Polkadot builder Docker images in Google Container Registry
- Polkadot Debian packages in Google Artifact Registry

## Usage

### Via GitHub UI

1. Go to Actions → Zombie Builder
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### When to Update

The builder images and binaries should be updated:
- When a new version of Polkadot is released
- When compatibility with a specific version is needed for testing
- When dependencies need to be updated
- When preparing for a release that requires testing against a specific Polkadot version
