#!/bin/sh
set -eu
case "$(uname -s)" in Linux) ;; *) echo 'Unsupported environment: Linux only'; exit 2;; esac
root=$(CDPATH='' cd -- "$(dirname -- "$0")/../.." && pwd -P)
exec bash "$root/test/eval/runner.sh" "$@"
