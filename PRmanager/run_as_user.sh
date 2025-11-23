#!/bin/sh

# Exit on any error and treat unset variables as errors
set -euo

OLD_UID=$1
OLD_GID=$2
shift; shift

# Create group if it doesn't exist
if ! getent group $OLD_GID > /dev/null 2>&1; then
    groupadd --gid $OLD_GID --non-unique user
fi

# Create user if it doesn't exist
if ! id -u $OLD_UID > /dev/null 2>&1; then
    useradd --uid $OLD_UID --gid $OLD_GID --non-unique --create-home --home-dir /home/user user
fi

export HOME=/home/user
if [ ! -d "$HOME" ]; then
    mkdir -p "$HOME"
    chown "$OLD_UID:$OLD_GID" "$HOME"
fi

# Ensure user has access to working directory
if [ -n "${PWD:-}" ]; then
    chown -R "$OLD_UID:$OLD_GID" "$PWD" 2>/dev/null || true
fi

sudo -E -u user "$@"

