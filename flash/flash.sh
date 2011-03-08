#!/bin/sh

set -e

make -C ../src flickernoise.fbiz

BATCH_FILE=flash.batch

ADDRESS=0x920000
if [ "$1" == "rescue" ]; then
    ADDRESS=0x2E0000
fi

batch() {
    echo -e "$1" >> "${BATCH_FILE}"
}

rm -rf ${BATCH_FILE}

batch "cable milkymist"
batch "detect"
batch "instruction CFG_OUT 000100 BYPASS"
batch "instruction CFG_IN 000101 BYPASS"
batch "pld load fjmem.bit"
batch "initbus fjmem opcode=000010"
batch "frequency 6000000"
batch "detectflash 0"
batch "endian big"
batch "flashmem ${ADDRESS} ../src/flickernoise.fbiz noverify"

jtag -n ${BATCH_FILE}
