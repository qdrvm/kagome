#!/bin/bash

set -eo pipefail

USER_ID=${USER_ID:-1000}
GROUP_ID=${GROUP_ID:-1000}
USER_NAME=${USER_NAME:-builder}
USER_GROUP=${USER_GROUP:-builder}

HOME_DIR="/home/$USER_NAME"
MOUNTED_DIRS_ARRAY=()

if [ ! -d "$HOME_DIR" ]; then
  mkdir -p "$HOME_DIR"
fi

if [ -n "$MOUNTED_DIRS" ]; then
  read -r -a MOUNTED_DIRS_ARRAY <<< "$MOUNTED_DIRS"
fi

echo "Current IDs for $USER_NAME: UID=$(id -u "$USER_NAME"), GID=$(id -g "$USER_NAME")"

if ! getent group "$GROUP_ID" &>/dev/null; then
  groupadd -g "$GROUP_ID" "$USER_GROUP"
else
  echo "Group $GROUP_ID already exists"
  USER_GROUP=$(getent group "$GROUP_ID" | cut -d: -f1)
fi

if ! id -u "$USER_ID" &>/dev/null; then
  useradd -u "$USER_ID" -g "$GROUP_ID" -s /bin/bash -m "$USER_NAME"
else
  echo "User $USER_ID already exists"
  USER_NAME=$(getent passwd "$USER_ID" | cut -d: -f1)
  usermod -d "$HOME_DIR" "$USER_NAME"
fi

chown -R "$USER_ID":"$GROUP_ID" "$HOME_DIR"

for dir in "${MOUNTED_DIRS_ARRAY[@]}"; do
  if [ -d "$dir" ]; then
    echo "Changing ownership of $dir to $USER_NAME:$USER_GROUP"
    chown -R "$USER_ID":"$GROUP_ID" "$dir"
  else
    echo "Directory $dir does not exist"
  fi
done

touch /tmp/entrypoint_done

echo "Switching to user $USER_NAME"
exec gosu "$USER_NAME" "$@"
