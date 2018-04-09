#!/bin/sh
# This file is part of Kvazaar HEVC encoder.
#
# Copyright (C) 2013-2016 Tampere University of Technology and others (see
# COPYING file).
#
# Kvazaar is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 2.1 as
# published by the Free Software Foundation.
#
# Kvazaar is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.

# This script updates parameter documentation in ../README.md file.

LANG=C
set -e

cd "$(dirname "$0")"

tmpfile="$(mktemp)"
readme_file="../README.md"

{
    sed '/BEGIN KVAZAAR HELP MESSAGE/q' -- "$readme_file";
    printf '```\n';
    ../src/kvazaar --help;
    printf '```\n';
    sed -n '/END KVAZAAR HELP MESSAGE/{:a;p;n;ba}' -- "$readme_file";
} >> "$tmpfile"

mv -- "$tmpfile" "../README.md"
