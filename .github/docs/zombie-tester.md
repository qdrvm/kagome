# Zombie Tester Pipeline

This pipeline builds Docker images used for running Zombienet tests.

## Purpose

The Zombie Tester Pipeline:
- Creates Docker images for running Zombienet tests
- Builds images for multiple architectures (AMD64, ARM64)
- Pushes images to Google Container Registry
- Creates multi-architecture manifests

## When It Runs

This pipeline can run:
- Manually through workflow_dispatch
- Scheduled runs can be enabled (commented out by default)

## Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `zombie_tester_tag` | Custom Zombie Tester tag | `latest` |
| `zombienet_release` | Custom zombienet release | `1.3.127` |
| `polkadot_package_version` | Custom Polkadot package version | `20250323-24123` |

## Jobs

1. `build_zombie_tester` - Builds the tester images for each architecture
2. `zombie_tester_manifest` - Creates and pushes multi-architecture Docker manifests

## Outputs/Artifacts

- Tester Docker images in Google Container Registry

## Usage

### Via GitHub UI

1. Go to Actions → Zombie Tester
2. Click "Run workflow"
3. Configure parameters as needed
4. Click "Run workflow"

### When to Update

The tester images should be updated:
- When a new version of Zombienet is released
- When a new version of Polkadot is needed for testing
- When test dependencies are updated
- When test frameworks need to be modified
