#!/bin/sh
set -e
make -C ../src flickernoise.fbiz
jtag -n flash-rescue.batch
