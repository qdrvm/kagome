# Kagome Zombienet Testing Documentation

This document provides comprehensive information about using Kagome's Zombienet testing system with Polkadot. The system enables automated parachain network testing, cross-chain compatibility validation, and integration verification.

## Table of Contents

- [Quick Start](#quick-start)
- [Environment Setup](#environment-setup)
- [Basic Usage](#basic-usage)
- [Available Tests](#available-tests)
- [Building Polkadot](#building-polkadot)
- [Tester Images](#tester-images)
- [Environment Variables Reference](#environment-variables-reference)
- [Common Workflows](#common-workflows)
- [Makefile Structure](#makefile-structure)
- [Performance Optimization](#performance-optimization)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

## Quick Start

For the impatient, here's how to get started with Zombienet testing:

```bash
# Set required environment variables
export DOCKER_REGISTRY_PATH="gcr.io/your-project/"
export PROJECT_ID="your-project-id"
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/credentials.json"

# Initialize version files
make configure

# Build Polkadot binary
make polkadot_binary

# Upload the package
make upload_apt_package

# Run a specific test
make test-polkadot-functional-0001-parachains-pvf
```

## Environment Setup

### Required Environment Variables

Before using the Zombienet testing system, set the following environment variables:

```bash
# Required for all operations
export DOCKER_REGISTRY_PATH="your-registry-path/"  # Include trailing slash
export PROJECT_ID="your-gcp-project-id"
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/your/gcp-credentials.json"
```

### Initial Configuration

Generate version files before first use:

```bash
make configure
```

This creates the necessary version files based on your current repository state:
- `commit_hash.txt` - Git commit information
- `kagome_version.txt` - Kagome version
- `polkadot-sdk-versions.txt` - Polkadot SDK version information
- `zombienet-versions.txt` - Zombienet version information

## Basic Usage

### Building Polkadot

Build Polkadot binaries for testing:

```bash
make polkadot_binary
```

### Uploading Polkadot Package

After building, upload the Polkadot binary package to Google Cloud Artifacts Repository:

```bash
make upload_apt_package
```

This step is necessary before running tests so that the test containers can install the package.

### Building Zombie Tester

Build the Zombie Tester image that runs the tests:

```bash
make zombie_tester
```

### Running Tests

Run a specific Zombienet test:

```bash
make test-polkadot-functional-0001-parachains-pvf
```

## Available Tests

The system includes multiple Zombienet tests that validate different aspects of Kagome's functionality:

### Polkadot Functional Tests

```bash
# PVF (Para Validation Function) testing
make test-polkadot-functional-0001-parachains-pvf

# Parachain disputes testing
make test-polkadot-functional-0002-parachains-disputes

# BEEFY and MMR testing
make test-polkadot-functional-0003-beefy-and-mmr

# Parachain garbage candidate testing
make test-polkadot-functional-0004-parachains-garbage-candidate

# Parachain disputes past session testing
make test-polkadot-functional-0005-parachains-disputes-past-session

# Parachain max tranche0 testing
make test-polkadot-functional-0006-parachains-max-tranche0

# Dispute freshly finalized testing
make test-polkadot-functional-0007-dispute-freshly-finalized

# Dispute old finalized testing
make test-polkadot-functional-0008-dispute-old-finalized

# Approval voting coalescing testing
make test-polkadot-functional-0009-approval-voting-coalescing

# Validator disabling testing
make test-polkadot-functional-0010-validator-disabling

# Async backing 6 seconds rate testing
make test-polkadot-functional-0011-async-backing-6-seconds-rate

# Systematic chunk recovery testing
make test-polkadot-functional-0013-systematic-chunk-recovery
```

### Custom Tests

```bash
# Validators warp sync testing
make test-custom-0001-validators-warp-sync
```

## Building Polkadot

### Basic Build

Build the Polkadot binary:

```bash
make polkadot_binary
```

This builds the following executables:
- polkadot
- polkadot-parachain
- malus
- undying-collator
- adder-collator
- polkadot-execute-worker
- polkadot-prepare-worker

### Build Customization

You can customize the Polkadot build:

```bash
# Build for a specific architecture
make polkadot_binary PLATFORM=linux/arm64 ARCHITECTURE=arm64
```

### Upload Package

Upload the Polkadot binary package to Google Cloud Storage:

```bash
make upload_apt_package
```

### Polkadot Builder

Build the Polkadot builder image:

```bash
make polkadot_builder
```

For all architectures:

```bash
make polkadot_builder_all_arch
```

## Tester Images

### Building Zombie Tester

Build the Zombie Tester image:

```bash
make zombie_tester
```

For all architectures:

```bash
make zombie_tester_all_arch
```

### Pushing Zombie Tester

Push the Zombie Tester image to the registry:

```bash
make zombie_tester_push
```

Create and push manifests:

```bash
make zombie_tester_push_manifest
```

## Environment Variables Reference

The system supports many environment variables to customize builds and tests:

### Base System Variables

```bash
# OS image configuration
OS_IMAGE_NAME=ubuntu            # Base OS image
OS_IMAGE_TAG=24.04             # OS version tag

# Compiler versions
RUST_VERSION=1.81.0            # Rust compiler version

# Platform settings
PLATFORM=linux/amd64           # Target platform
ARCHITECTURE=amd64             # Target architecture
```

### Docker Registry Variables

```bash
# Container registry settings
DOCKER_REGISTRY_PATH=gcr.io/your-project/  # Registry path with trailing slash
```

### Polkadot Variables

```bash
# Polkadot SDK settings
POLKADOT_SDK_TAG=               # Optional custom tag
POLKADOT_SDK_RELEASE=v1.0.0     # Polkadot SDK release version
SCCACHE_VERSION=0.9.0           # Build cache version
SCCACHE_GCS_BUCKET=             # GCS bucket for build cache
```

### Test Variables

```bash
# Test settings
COPY_LOGS_TO_HOST=true         # Whether to copy logs to host
HOST_LOGS_PATH=/tmp/test_logs  # Where to store logs on host
DELETE_IMAGE_AFTER_TEST=true   # Delete container after test
```

### Google Cloud Variables

```bash
# GCP settings
PROJECT_ID=your-gcp-project    # GCP project ID
REGION=europe-north1           # GCP region
ARTIFACTS_REPO=kagome-apt     # Artifact repository name
```

## Common Workflows

### Development Testing Workflow

For testing during development:

```bash
# Initial setup (once)
export DOCKER_REGISTRY_PATH="gcr.io/your-project/"
export PROJECT_ID="your-project-id"
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/credentials.json"
make configure

# Build the tester image (occasionally when dependencies change)
make zombie_tester

# Run specific test during development
make test-polkadot-functional-0001-parachains-pvf
```

### CI/CD Pipeline Workflow

For CI/CD systems:

```bash
# Setup
make configure

# Build and upload Polkadot binaries
make polkadot_binary
make upload_apt_package

# Build tester image if needed
make zombie_tester

# Run tests in parallel
make test-polkadot-functional-0001-parachains-pvf &
make test-polkadot-functional-0002-parachains-disputes &
make test-polkadot-functional-0003-beefy-and-mmr &
wait
```

### Cross-Platform Testing Workflow

For testing across architectures:

```bash
# AMD64 build and test
make polkadot_binary PLATFORM=linux/amd64 ARCHITECTURE=amd64
make zombie_tester PLATFORM=linux/amd64 ARCHITECTURE=amd64
make test-polkadot-functional-0001-parachains-pvf

# ARM64 build and test
make polkadot_binary PLATFORM=linux/arm64 ARCHITECTURE=arm64
make zombie_tester PLATFORM=linux/arm64 ARCHITECTURE=arm64
make test-polkadot-functional-0001-parachains-pvf
```

## Makefile Structure

The Zombienet testing system is organized into modular Makefiles for better maintainability:

```
Makefile                    # Main entry point with basic checks
├── Makefile.include/       # Directory containing specialized Makefiles
│   ├── variables.mk        # Common variables and configuration
│   ├── versions.mk         # Version handling for Polkadot and Zombienet
│   ├── polkadot.mk         # Polkadot binary building logic
│   ├── polkadot_image.mk   # Polkadot builder image creation
│   ├── zombie_tester.mk    # Zombie tester image creation
│   ├── run_zombietest.mk   # Logic for running Zombienet tests
│   └── zombietests.mk      # Individual test targets
```

## Performance Optimization

### Parallelize Test Execution

For running multiple tests in parallel, use background processes:

```bash
# Run tests in parallel and wait for all to complete
make test-polkadot-functional-0001-parachains-pvf &
make test-polkadot-functional-0002-parachains-disputes &
make test-polkadot-functional-0003-beefy-and-mmr &
wait
```

### Preserve Test Containers

When debugging or iterating, keep containers for faster subsequent tests:

```bash
# Keep the test container after completion
make test-polkadot-functional-0001-parachains-pvf DELETE_IMAGE_AFTER_TEST=false
```

### Optimize Docker Resources

Configure Docker with adequate CPU and memory:

```bash
# On Linux
docker info
# Check and adjust /etc/docker/daemon.json for resource limits

# On macOS/Windows
# Use Docker Desktop settings to adjust available resources
```

### Use Architecture-specific Builds

Build and test on your native architecture for best performance:

```bash
# For x86_64/amd64 systems
make polkadot_binary PLATFORM=linux/amd64 ARCHITECTURE=amd64

# For ARM64 systems (M1/M2 Macs, etc.)
make polkadot_binary PLATFORM=linux/arm64 ARCHITECTURE=arm64
```

## Troubleshooting

If you encounter issues with the Zombienet testing system:

### Common Issues and Solutions

#### Test Timeout Errors

**Problem**: Tests fail with timeout errors.

**Solution**: Tests may require more time on resource-constrained systems.

#### Missing Polkadot or Kagome Packages

**Problem**: Tests fail with package installation errors.

**Solution**: Verify that the required packages have been built and uploaded.

#### Build Failures

**Problem**: Polkadot binary fails to build.

**Solution**: Check logs and ensure dependencies are available.

#### Failed Health Checks

**Problem**: Container health checks report failures.

**Solution**: Ensure Docker has enough resources and check container logs.

#### Runtime Cache Issues

**Problem**: Tests fail due to missing or corrupted runtime files.

**Solution**: Check the runtime cache directory and Kagome installation.

### Accessing Test Logs

Test logs are automatically copied to the host if `COPY_LOGS_TO_HOST=true` (default):

```bash
# View test logs
ls -la /tmp/test_logs/
```

If `DELETE_IMAGE_AFTER_TEST=false`, you can access the container directly:

```bash
# Access the container
docker exec -it CONTAINER_NAME bash

# View logs inside container
ls -la /tmp/zombie-*/logs/
```

## FAQ

### General Questions

**Q: What is Zombienet?**  
**A:** Zombienet is a testing framework for Polkadot-based blockchains that automates the setup and testing of parachain networks.

**Q: Can I add my own tests?**  
**A:** Yes, create a new test file in the appropriate directory and add a target in `Makefile.include/zombietests.mk`.

**Q: How long do tests typically take to run?**  
**A:** Test duration varies, but most functional tests take 5-15 minutes depending on system performance.

**Q: Can I use Zombienet based on docker without Google Cloud?**  
**A:** Yes, but you'll need to modify the Makefiles to use local storage for artifacts instead of GCP.

**Q: Can I use this with a custom Kagome build?**  
**A:** Yes, build and upload your custom Kagome package, then reference it in the test using `KAGOME_PACKAGE_VERSION`.

### Test-Specific Questions

**Q: What does the PVF test validate?**  
**A:** It tests the Para Validation Function execution in a parachain context, ensuring correct validation of parachain blocks.

**Q: Why are there multiple dispute tests?**  
**A:** They test different dispute resolution scenarios - recent disputes, old disputes, and disputes with different validator configurations.

**Q: What are the hardware requirements for these tests?**  
**A:** Recommendations:
- CPU: 4+ cores
- RAM: 8GB+ (16GB recommended for parallel tests)
- Storage: 20GB+ free space
- Network: Stable internet connection for cloud artifact access

**Q: Can I run tests against a non-standard Polkadot version?**  
**A:** Yes, use the `POLKADOT_SDK_TAG` variable to specify a custom tag/branch when building:
```bash
make polkadot_binary POLKADOT_SDK_TAG=custom-branch
```

### Advanced Testing

**Q: Can I modify test parameters?**  
**A:** Yes, edit the test files in the Zombienet test directory or use environment variables to override test settings.

**Q: How do I debug a failed test?**  
**A:** Set `DELETE_IMAGE_AFTER_TEST=false`, then examine logs in the test container and use interactive debugging tools.

**Q: Can I run custom Zombienet test scripts?**  
**A:** Yes, create a custom test script and reference it in the `run_test` function:
```bash
$(call run_test, "path/to/your/custom-test.zndsl")
```

**Q: How do I test with multiple Kagome versions simultaneously?**  
**A:** Run separate test containers with different `KAGOME_PACKAGE_VERSION` values.

**Q: Can I customize the Polkadot build for specific tests?**  
**A:** Yes, build Polkadot with custom features or flags by modifying the `BUILD_COMMANDS` variable.

**Q: How do I analyze test performance?**  
**A:** Review the test execution times reported in the logs and use Docker stats to monitor resource usage during tests.
```bash
docker stats
```

