#!/usr/bin/env bash

set -e

if [[ $# -ne 1 && $# -ne 2 ]]
then
    echo "Invalid number of arguments to update.git.sh."
    exit 1
fi

toolchain=${1^^}
pr=0
if [[ $# -eq 2 && $2 == "--pr" ]]
then
    pr=1
fi

source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

function get() {
    cat "${source_dir}/../toolchain/config.cmake" \
        | grep REAVEROS_${toolchain}_$1 \
        | awk '{ print $2 }' | sed 's/.$//'
}

repo=$(get REPO)
tag=$(get TAG)
tag_pattern=$(get PATTERN)
tag_exclude=$(get EXCLUDE)

latest_tag=$(git ls-remote -t ${repo} \
    | awk '{ print $2 }' | sed 's:refs/tags/::' \
    | egrep "^${tag_pattern}(\^\{\})?$" \
    | egrep -v "^${tag_exclude}(\^\{\})?$" \
    | sort -V | sed 's/\^{}//' | tail -n1)

if [[ ${tag} != ${latest_tag} ]]
then
    echo "$1: upgrade available: $tag -> $latest_tag"

    if [[ $pr -eq 1 ]]
    then
        set -x
        "${source_dir}/create-pr" "${1}" TAG "${latest_tag}"
    fi
fi

