# Kagome CI Pipeline Documentation

This directory contains documentation for the CI pipelines used in the Kagome project.

## Main Pipelines

- [Main Pipeline](main-pipeline.md) - The primary pipeline that orchestrates all testing and build processes
- [Build Pipeline](build.md) - Handles building Kagome binaries and Docker images
- [Tidy Pipeline](tidy.md) - Runs code quality checks with clang-tidy
- [Sanitizers Pipeline](sanitizers.md) - Builds with various sanitizers (TSAN, ASAN, UBSAN)
- [Zombie Tests](zombie-tests.md) - Runs integration tests using Zombienet

## Infrastructure Pipelines

- [Kagome Runtime](kagome-runtime.md) - Builds runtime cache packages
- [Kagome Builder](kagome-builder.md) - Creates builder Docker images
- [Zombie Tester](zombie-tester.md) - Creates tester Docker images for zombienet
- [Zombie Builder](zombie-builder.md) - Creates Polkadot builder images for zombienet

## Workflow

The recommended workflow is to use the Main Pipeline, which will coordinate all other pipelines as needed. Individual pipelines can be run directly for more specific tasks or debugging.

## Architecture Support

Most pipelines support both AMD64 and ARM64 architectures. By default, the pipelines run on AMD64 only. ARM64 support can be enabled with the appropriate input parameters.
