#!/bin/bash

set -eo pipefail

install_packages kagome-dev=$KAGOME_PACKAGE_VERSION kagome-dev-runtime
echo "KAGOME_DEV_VERSION=`apt-cache policy kagome-dev | grep 'Installed:' | awk '{print $2}'`"
echo "KAGOME_DEV_RUNTIME_VERSION=`apt-cache policy kagome-dev-runtime | grep 'Installed:' | awk '{print $2}'`"

sed -i '1s/^/#/' /etc/apt/sources.list.d/kagome.list

exec gosu $USER_NAME "$@"
