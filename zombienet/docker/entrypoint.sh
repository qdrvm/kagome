#!/bin/bash

set -eo pipefail

MOUNTED_DIRS=(
  "/home/${USER_NAME}" 
)

for dir in "${MOUNTED_DIRS[@]}"; do
  if [ -d "$dir" ]; then
    echo "Change rights for $dir"
    chown -R $USER_ID:$GROUP_ID "$dir"
  else
    echo "Directory $dir does not exist"
  fi
done

exec gosu $USER_NAME "$@"
