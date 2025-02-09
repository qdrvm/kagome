#!/bin/bash

set -eo pipefail

sed -i 's/^#//' /etc/apt/sources.list.d/kagome.list
install_packages kagome-dev=$KAGOME_PACKAGE_VERSION kagome-dev-runtime
sed -i '1s/^/#/' /etc/apt/sources.list.d/kagome.list

echo "KAGOME_DEV_VERSION=`apt-cache policy kagome-dev | grep 'Installed:' | awk '{print $2}'`"
echo "KAGOME_DEV_RUNTIME_VERSION=`apt-cache policy kagome-dev-runtime | grep 'Installed:' | awk '{print $2}'`"


exec gosu $USER_NAME "$@"
