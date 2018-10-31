#/bin/bash

if [ $# -ne 1 ]; then
  echo "usage: $0 EXPORT_NAME" 2>&1
  exit 1
fi

script_dir="$(dirname "$(readlink -f "$0")")"
export_file="$1.tar.gz"

tarignore_tmp="$(mktemp)"
cp "$script_dir/.tarignore" "$tarignore_tmp"
echo "$export_file" >> "$tarignore_tmp"

tar --exclude-from="$tarignore_tmp" -vczf "$export_file" "$script_dir"/*
