#!/bin/bash
set -e

MOUNT_POINT="${1:-/mnt/remote}"
NFS_SERVER="localhost"
# fsid=0 makes /mnt/test the NFSv4 pseudo-root, so mount path is /
NFS_EXPORT="/"

# Allow NFS mounts from containers (Fedora/SELinux)
if command -v setsebool &>/dev/null; then
    echo "Configuring SELinux for NFS..."
    sudo setsebool -P virt_use_nfs 1
fi

# Install nfs-utils if missing
if ! command -v mount.nfs4 &>/dev/null; then
    echo "Installing nfs-utils..."
    sudo dnf install -y nfs-utils 2>/dev/null || sudo apt-get install -y nfs-common
fi

sudo mkdir -p "$MOUNT_POINT"
sudo mount -t nfs4 "$NFS_SERVER:$NFS_EXPORT" "$MOUNT_POINT"
echo "Mounted $NFS_SERVER:$NFS_EXPORT at $MOUNT_POINT"
