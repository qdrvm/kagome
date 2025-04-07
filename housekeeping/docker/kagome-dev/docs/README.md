# Kagome Build System Documentation

This document provides comprehensive information about using the Kagome build system with Makefiles. The system supports multiple build types, architectures, sanitizer builds, caching, and Docker image management.

## Table of Contents

- [Quick Start](#quick-start)
- [Environment Setup](#environment-setup)
- [Basic Usage](#basic-usage)
- [Build Types](#build-types)
- [Docker Images](#docker-images)
- [Sanitizer Builds](#sanitizer-builds)
- [Cache Management](#cache-management)
- [Code Quality Tools](#code-quality-tools)
- [Runtime Packages](#runtime-packages)
- [Environment Variables Reference](#environment-variables-reference)
- [Common Workflows](#common-workflows)
- [Makefile Structure](#makefile-structure)
- [Performance Optimization](#performance-optimization)
- [Security Considerations](#security-considerations)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)

## Quick Start

For the impatient, here's how to get started with Kagome build system:

```bash
# Set required environment variables
export DOCKER_REGISTRY_PATH="gcr.io/your-project/"
export PROJECT_ID="your-project-id"
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/credentials.json"

# Initialize version files
make configure

# Build Kagome (Release build)
make kagome_docker_build

# Upload the package
make upload_apt_package

# Build Docker image
make kagome_image_build

# Done! Your image is ready at:
# gcr.io/your-project/kagome_release:[commit-hash]-[arch]
```

## Environment Setup

### Required Environment Variables

Before using the build system, set the following environment variables:

```bash
# Required for all operations
export DOCKER_REGISTRY_PATH="your-registry-path/"  # Include trailing slash
export PROJECT_ID="your-gcp-project-id"
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/your/gcp-credentials.json"

# Optional - for GitHub Hunter access (improves dependency fetching)
export GITHUB_HUNTER_USERNAME="your-github-username"
export GITHUB_HUNTER_TOKEN="your-github-token"
```

### Initial Configuration

Generate version files before first use:

```bash
make configure
```

This creates the necessary `commit_hash.txt` and `kagome_version.txt` files based on your current git repository state.

## Basic Usage

### Building Kagome

Build Kagome with default settings (Release build for current architecture):

```bash
make kagome_docker_build
```

### Upload Package

Upload the built package to Google Cloud Storage:

```bash
make upload_apt_package
```

### Complete Build Pipeline

Run the entire build process:

```bash
make kagome_docker_build upload_apt_package
```

### Customizing the Build

```bash
# Set build type (Debug, Release, RelWithDebInfo)
make kagome_docker_build BUILD_TYPE=Debug

# Build for a specific architecture
make kagome_docker_build PLATFORM=linux/arm64 ARCHITECTURE=arm64

# Enable warnings as errors
make kagome_docker_build WERROR=ON

# Control number of build threads
make kagome_docker_build BUILD_THREADS=8
```

## Build Types

The system supports three build types:

```bash
# Debug build with minimal optimization
make kagome_docker_build BUILD_TYPE=Debug

# Release build (optimized, no debug info)
make kagome_docker_build BUILD_TYPE=Release

# Release build with debug information
make kagome_docker_build BUILD_TYPE=RelWithDebInfo
```

## Docker Images

### Building Images

```bash
# Build a Docker image
make kagome_image_build

# Build images for all supported architectures
make kagome_image_build_all_arch
```

### Pushing Images

```bash
# Push to GCR
make kagome_image_push

# Push to DockerHub
make kagome_image_push_dockerhub

# Build and push for all architectures to both registries
make kagome_image_build_all_arch_dockerhub
```

### Image Information

```bash
# Show information about available images
make kagome_image_info
make kagome_builder_image_info
```

## Sanitizer Builds

### Individual Sanitizer Builds

```bash
# Address Sanitizer (ASAN)
make kagome_docker_build_asan

# Thread Sanitizer (TSAN)
make kagome_docker_build_tsan

# Undefined Behavior Sanitizer (UBSAN)
make kagome_docker_build_ubsan

# Combined ASAN+UBSAN
make kagome_docker_build_asanubsan
```

### Complete Sanitizer Workflows

Run the entire workflow for a specific sanitizer (build, package, upload, and create Docker image):

```bash
make kagome_asan_all
make kagome_tsan_all
make kagome_ubsan_all
make kagome_asanubsan_all
```

### Build All Sanitizers

```bash
make kagome_sanitizers_all
```

## Cache Management

### Basic Cache Commands

```bash
# Show cache information
make cache_info

# List available caches
make cache_list

# Upload current build cache
make cache_upload

# Download and use existing cache
make cache_get

# Check if upload is needed and upload if necessary
make cache_check_and_upload
```

### Sanitizer Cache Management

Each sanitizer has specific cache targets:

```bash
# ASAN cache management
make kagome_cache_upload_asan
make kagome_cache_get_asan

# TSAN cache management
make kagome_cache_upload_tsan
make kagome_cache_get_tsan

# UBSAN cache management
make kagome_cache_upload_ubsan
make kagome_cache_get_ubsan

# ASAN+UBSAN cache management
make kagome_cache_upload_asanubsan
make kagome_cache_get_asanubsan
```

### Cache Configuration

Control cache behavior with environment variables:

```bash
# Set cache parameters
make cache_upload CACHE_UPLOAD_ALLOWED=true CACHE_LIFETIME_DAYS=7 ZSTD_LEVEL=3
```

## Code Quality Tools

### Clang-Tidy

Run clang-tidy on the codebase to check for common issues:

```bash
make kagome_dev_docker_build_tidy
```

## Runtime Packages

### Building Runtime Cache

```bash
make runtime_cache
```

### Uploading Runtime Package

```bash
make upload_apt_package_runtime
```

## Environment Variables Reference

The build system supports many environment variables to customize builds. Here are the most important ones:

### Base System Variables

```bash
# OS image configuration
OS_IMAGE_NAME=ubuntu            # Base OS image
OS_IMAGE_TAG=24.04             # OS version tag
OS_REL_VERSION=noble           # OS release name

# Compiler versions
RUST_VERSION=1.81.0            # Rust compiler version
GCC_VERSION=13                 # GCC version
LLVM_VERSION=19                # LLVM/Clang version

# Build system settings
BUILD_THREADS=8                # Number of build threads (default: auto-detected)
WERROR=ON                      # Treat warnings as errors (ON/OFF)
USER_ID=$(id -u)              # Current user ID for container permissions
GROUP_ID=$(id -g)             # Current group ID for container permissions
```

### Docker Registry Variables

```bash
# Container registry settings
DOCKER_REGISTRY_PATH=gcr.io/your-project/  # Registry path with trailing slash
DOCKERHUB_REGISTRY_PATH=qdrvm/kagome       # DockerHub registry path
BUILDER_IMAGE_TAG=ub2404-7229784_rust1.81.0_gcc13_llvm19  # Builder image tag
```

### Build Type Variables

```bash
# Build configuration
BUILD_TYPE=Release             # Build type (Debug/Release/RelWithDebInfo)
PLATFORM=linux/amd64           # Target platform
ARCHITECTURE=amd64             # Target architecture (derived from PLATFORM)
```

### Sanitizer Variables

```bash
# Sanitizer build parameters (customize if needed)
BUILD_TYPE_SANITIZERS=RelWithDebInfo  # Build type for sanitizer builds

# Sanitizer tool parameters
ASAN_PARAMS=-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-13_cxx20.cmake -DASAN=ON
TSAN_PARAMS=-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/clang-19_cxx20.cmake -DTSAN=ON
UBSAN_PARAMS=-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/clang-19_cxx20.cmake -DUBSAN=ON -DUBSAN_TRAP=OFF -DUBSAN_ABORT=ON
ASANUBSAN_PARAMS=-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/gcc-13_cxx20.cmake -DASAN=ON -DUBSAN=ON -DUBSAN_TRAP=OFF -DUBSAN_ABORT=ON
```

### Cache Management Variables

```bash
# Cache configuration
CACHE_BUCKET=ci-cache-kagome           # GCP bucket for build caches
CACHE_UPLOAD_ALLOWED=false             # Allow cache uploads (true/false)
CACHE_LIFETIME_DAYS=3                  # Cache lifetime in days
ZSTD_LEVEL=5                           # Compression level (1-19)
CACHE_ONLY_MASTER=false                # Only upload cache for master branch
```

### Google Cloud Variables

```bash
# GCP settings
PROJECT_ID=your-gcp-project           # GCP project ID
REGION=europe-north1                  # GCP region
ARTIFACTS_REPO=kagome-apt            # Artifact repo name
PUBLIC_ARTIFACTS_REPO=kagome         # Public artifact repo
```

## Common Workflows

### Development Workflow

For local development, a typical workflow might look like:

```bash
# Initial setup (once)
export DOCKER_REGISTRY_PATH="gcr.io/your-project/"
export PROJECT_ID="your-project-id"
export GOOGLE_APPLICATION_CREDENTIALS="/path/to/credentials.json"
make configure

# Build with debug symbols for development
make kagome_docker_build BUILD_TYPE=RelWithDebInfo WERROR=OFF

# Once built, create and push image
make kagome_image_build
make kagome_image_push
```

### CI/CD Pipeline Workflow

For CI/CD systems, you might use:

```bash
# Setup
make configure

# Use cache to speed up builds
make cache_get || echo "No cache found, building from scratch"
make kagome_docker_build

# Upload new cache for future builds
make cache_check_and_upload

# Create and push images
make kagome_image_build_all_arch_dockerhub
```

### Cross-Platform Testing Workflow

For testing across architectures:

```bash
# AMD64 build and test
make kagome_docker_build PLATFORM=linux/amd64 ARCHITECTURE=amd64
make upload_apt_package
make kagome_image_build

# ARM64 build and test
make kagome_docker_build PLATFORM=linux/arm64 ARCHITECTURE=arm64
make upload_apt_package
make kagome_image_build

# Create multi-arch manifest
make kagome_image_push_manifest
```

### Sanitizer Testing Workflow

For comprehensive memory/thread safety testing:

```bash
# Memory error detection with ASAN
make kagome_cache_get_asan || echo "No ASAN cache found"
make kagome_docker_build_asan
make kagome_cache_check_and_upload_asan

# Thread safety testing with TSAN 
make kagome_cache_get_tsan || echo "No TSAN cache found"
make kagome_docker_build_tsan
make kagome_cache_check_and_upload_tsan

# Check all sanitizers in one go
make kagome_sanitizers_all
```

### Release Workflow

For preparing a release:

```bash
# Build optimized release version
make kagome_docker_build BUILD_TYPE=Release

# Create release packages and images for all architectures
make kagome_image_build_all_arch_dockerhub

# Build runtime packages
make runtime_cache
make upload_apt_package_runtime
```

## Makefile Structure

The build system is organized into modular Makefiles for better maintainability:

```bash
# Main Makefile
Makefile

# Docker-related targets
docker/Makefile

# Cache management targets
cache/Makefile

# Sanitizer build targets
sanitizers/Makefile

# Code quality tools
quality/Makefile
```

Each Makefile is designed to handle specific tasks, making it easier to extend and maintain the build system.

## Performance Optimization

To optimize build performance:

1. Use caching effectively (`make cache_get`).
2. Increase build threads (`BUILD_THREADS`).
3. Use optimized compiler flags for release builds.
4. Minimize Docker image layers to reduce build time.
5. Use multi-stage Docker builds for cleaner images.

## Security Considerations

1. Ensure GCP credentials are stored securely and not hardcoded.
2. Use private Docker registries for sensitive images.
3. Regularly update base images to include security patches.
4. Limit permissions for build containers.
5. Use environment variables to avoid exposing sensitive data in Makefiles.

## Troubleshooting

If you encounter issues:

1. Ensure all required environment variables are set correctly.
2. Verify Docker is properly configured and running.
3. Check that your GCP credentials are valid and have sufficient permissions.
4. For build failures, check the Docker container logs.
5. Try cleaning the build environment with `make reset_build_state`.
6. For cache-related issues, check that the GCS bucket is accessible.

### Common Errors

- **Missing version files**: Run `make configure`.
- **Container health check failures**: Check Docker resources and network connectivity.
- **Build failures**: Examine the output for compilation errors.
- **Upload failures**: Verify GCP credentials and permissions.
- **Cache access failures**: Check bucket permissions and GCP authentication.
- **Out of disk space**: Clean Docker images with `docker system prune`.
- **Out of memory during build**: Increase Docker memory allocation.
- **Permission issues**: Check USER_ID and GROUP_ID settings.

### Docker Container Debugging

If a build fails inside a container, you can debug it manually:

```bash
# See logs from failed build
docker logs $(CONTAINER_NAME)

# Access a running container for debugging
docker exec -it $(CONTAINER_NAME) bash

# Start a fresh container with the same mounts
docker run -it --rm \
  -v $(WORKING_DIR):/opt/kagome \
  -v $(CACHE_DIR)/.cargo/registry:$(IN_DOCKER_HOME)/.cargo/registry \
  -v $(CACHE_DIR)/.hunter:$(IN_DOCKER_HOME)/.hunter \
  $(DOCKER_REGISTRY_PATH)kagome_builder_deb:$(BUILDER_LATEST_TAG) \
  bash
```

## FAQ

### How do I customize the build?

Use environment variables like `BUILD_TYPE`, `PLATFORM`, and `ARCHITECTURE` to customize builds.

### Can I use Kagome without Docker?

Yes, but Docker simplifies dependency management and ensures consistent builds.

### How do I report issues?

Submit issues on the Kagome GitHub repository or contact the maintainers directly.

### Is Kagome production-ready?

Yes, Kagome is designed for production use, but ensure thorough testing for your specific use case.
