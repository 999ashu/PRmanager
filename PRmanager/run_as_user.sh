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
    useradd --uid $OLD_UID --gid $OLD_GID --non-unique user
fi

sudo -E -u user "$@"