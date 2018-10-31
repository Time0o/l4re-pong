#/bin/bash

if [ $# -ne 1 ]; then
  echo "usage: $0 EXPORT_NAME" 2>&1
  exit 1
fi

tarignore_tmp="$(mktemp)"
cp .tarignore "$tarignore_tmp"
echo "$1" >> "$tarignore_tmp"

tar --exclude-from="$tarignore_tmp" -vczf "$1" *
