#!/bin/sh
# Linux adaptation of the user-designated launcher; original remains untouched.
set -eu
case "$(uname -s)" in Linux) ;; *) echo 'Unsupported environment: Linux only' >&2; exit 2;; esac
library=$1
shift
[ "$#" -gt 0 ] || exit 2
export LD_PRELOAD="$library"
exec "$@"
