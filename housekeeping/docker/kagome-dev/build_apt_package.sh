#!/bin/bash

set -euo pipefail

# Function to display usage
usage() {
    echo "Usage: $0 <version> <architecture> <package_name> <artifacts_dir> <description> <dependencies>"
    echo "Example: $0 1.0 amd64 mypackage /path/to/artifacts 'libc6, libssl1.1'"
    exit 1
}

# Function to log messages
log() {
    local MESSAGE=$1
    echo "[INFO] $MESSAGE"
}

# Check for the correct number of arguments
if [ "$#" -ne 6 ]; then
    usage
fi

# Assign variables from arguments
VERSION=$1
ARCHITECTURE=$2
PACKAGE_NAME=$3
ARTIFACTS_DIR=$4
DESCRIPTION=$5
DEPENDENCIES=$6

# Define maintainer and homepage variables
MAINTAINER="Zak Fein <zak@qdrvm.io>"
HOMEPAGE="https://qdrvm.io"
DIR_NAME=${PACKAGE_NAME}_${VERSION}_${ARCHITECTURE}

# Validate parameters
log "Validating parameters..."

if [ -z "$VERSION" ]; then
    echo "Error: Version is required." >&2
    exit 1
fi

if [ -z "$ARCHITECTURE" ]; then
    echo "Error: Architecture is required." >&2
    exit 1
fi

if [ -z "$PACKAGE_NAME" ]; then
    echo "Error: Package name is required." >&2
    exit 1
fi

if [ -z "$ARTIFACTS_DIR" ] || [ ! -d "$ARTIFACTS_DIR" ]; then
    echo "Error: A valid artifacts directory is required." >&2
    exit 1
fi

if [ -z "$DESCRIPTION" ]; then
    echo "Error: Description is required." >&2
    exit 1
fi

if [ -z "$DEPENDENCIES" ]; then
    echo "Error: Dependencies are required." >&2
    exit 1
fi

log "Creating package directory structure..."
mkdir -p ./pkg/${DIR_NAME}/DEBIAN
mkdir -p ./pkg/${DIR_NAME}/usr/local/bin

log "Copying artifacts..."
mv -f ${ARTIFACTS_DIR}/* ./pkg/${DIR_NAME}/usr/local/bin/

echo "Package: ${PACKAGE_NAME}"
echo "Version: ${VERSION}"
echo "Maintainer: ${MAINTAINER}"
echo "Depends: ${DEPENDENCIES}"
echo "Architecture: ${ARCHITECTURE}"
echo "Homepage: ${HOMEPAGE}"
echo "Description: ${DESCRIPTION}"
echo "Artifacts: ${ARTIFACTS_DIR}"
echo "Directory: ./pkg/${DIR_NAME}/usr/local/bin/"

log "Creating control file..."
cat <<EOF > ./pkg/${DIR_NAME}/DEBIAN/control
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Maintainer: ${MAINTAINER}
Depends: ${DEPENDENCIES}
Architecture: ${ARCHITECTURE}
Homepage: ${HOMEPAGE}
Description: ${DESCRIPTION}
EOF

log "Building the package..."
dpkg --build ./pkg/${DIR_NAME}

log "Displaying package info..."
dpkg-deb --info ./pkg/${DIR_NAME}.deb

log "Package created successfully."
