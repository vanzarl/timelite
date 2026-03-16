#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -Werror"

cc $CFLAGS -g main.c -o timelite
