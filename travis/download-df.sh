#!/bin/sh

script_name="$(basename "$0")"
new_script_path="$(dirname "$0")/../ci/${script_name}"

printf >&2 "\nNote: travis/%s is deprecated. Use ci/%s instead.\n\n" "${script_name}" "${script_name}"

"${new_script_path}" "$@"
exit $?
