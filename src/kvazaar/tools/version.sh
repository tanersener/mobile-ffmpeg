#!/bin/sh

cd "$(dirname "$0")"
cd ..

if type git >/dev/null 2>/dev/null && [ -d .git ]; then
    version="$(git describe --dirty --tags --match 'v*')"
    version="${version} $(git log -1 --pretty=format:%cd --date=short)"
else
    version="v$(awk '/#define KVZ_VERSION/ { print $3 }' src/global.h)"
fi

printf '%s\n' "$version"
