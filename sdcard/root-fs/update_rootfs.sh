#!/bin/bash
#bash update_rootfs.sh rootfs.tar file_name root/
set -e

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 <rootfs.tar> <file_to_add> <path_inside_rootfs>"
    exit 1
fi

ROOTFS_TAR="$1"
FILE_SRC="$2"
DEST_PATH="$3"

# Remove leading slash
DEST_PATH="${DEST_PATH#/}"

if [[ ! -f "$ROOTFS_TAR" ]]; then
    echo "Error: rootfs tar file does not exist"
    exit 1
fi

if [[ ! -f "$FILE_SRC" ]]; then
    echo "Error: source file does not exist"
    exit 1
fi

TMPDIR=$(mktemp -d)
echo "Working in $TMPDIR"

# Extract rootfs
tar -xf "$ROOTFS_TAR" -C "$TMPDIR"

# Detect correct directory (tar usually uses ./root/)
if [[ -d "$TMPDIR/$DEST_PATH" ]]; then
    DEST="$TMPDIR/$DEST_PATH"
elif [[ -d "$TMPDIR/./$DEST_PATH" ]]; then
    DEST="$TMPDIR/./$DEST_PATH"
else
    echo "Error: destination path '$DEST_PATH' does not exist in rootfs"
    rm -rf "$TMPDIR"
    exit 1
fi

echo "Copying $FILE_SRC -> $DEST"
cp "$FILE_SRC" "$DEST"

# Repack rootfs
NEW_TAR="$(basename "$ROOTFS_TAR")"
tar -cf "$NEW_TAR" -C "$TMPDIR" .

echo "Created $NEW_TAR"
rm -rf "$TMPDIR"
