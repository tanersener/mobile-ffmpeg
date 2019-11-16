#!/bin/sh

git submodule update --init --depth 1
autoreconf -if
