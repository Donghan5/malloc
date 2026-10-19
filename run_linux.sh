#!/bin/sh
export LD_LIBRARY_PATH=.
export DYLD_INSERT_LIBRARIES=$PWD/libft_malloc.so
$@