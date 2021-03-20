#!/bin/bash

set -e

message_file=.git/COMMIT_EDITMSG

get_git_root() {
    git rev-parse --show-toplevel
}

write_msg() {
    echo "$@" >> "${message_file}"
}

git_root="$(get_git_root)"
cd "${git_root}"
rm -f "${message_file}"
write_msg "Auto-update submodules"
write_msg ""

cat ci/update-submodules.manifest | while read path branch; do
    cd "${git_root}/${path}"
    test "${git_root}" != "$(get_git_root)"
    git checkout "${branch}"
    git pull --ff-only
    cd "${git_root}"
    if ! git diff --quiet --ignore-submodules=dirty -- "${path}"; then
        git add "${path}"
        write_msg "${path}: ${branch}"
    fi
done

if ! git diff --exit-code --cached; then
    git commit --file "${message_file}" --no-edit
    exit 0
else
    exit 1
fi
