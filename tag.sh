#!/bin/bash

if [ $# -ne 1 ];
then
    echo "Usage: tag.sh <tag name>"
    exit 1
fi

git tag -d $1
git push origin ":refs/tags/"$1
git tag $1
