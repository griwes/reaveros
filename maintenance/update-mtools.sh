#!/usr/bin/env bash

set -e

if [[ $# -ne 0 && $# -ne 1 ]]
then
    echo "Invalid number of arguments to update-mtools.sh."
    exit 1
fi

pr=0
if [[ $# -eq 1 && $1 == "--pr" ]]
then
    pr=1
fi

source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

function get() {
    cat "${source_dir}/../toolchain/config.cmake" \
        | grep REAVEROS_MTOOLS_$1 \
        | awk '{ print $2 }' | sed 's/.$//'
}

dir=$(get DIR)
ver=$(get VER)
tag_pattern=$(get PATTERN)
tag_exclude=$(get EXCLUDE)

latest_ver=$(curl ${dir} 2>/dev/null \
    | awk '{ print $9 }' \
    | egrep "^${tag_pattern}$" \
    | egrep -v "^${tag_exclude}$" \
    | sort -V | tail -n1)

if [[ ${ver} != ${latest_ver} ]]
then
    echo "mtools: upgrade available: $ver -> $latest_ver"

    if [[ $pr -eq 1 ]]
    then
        "${source_dir}/create-pr" mtools VER "${latest_ver}"
    fi
fi
