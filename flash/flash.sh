#!/bin/bash

set -e

ADDRESS=0x920000
if [ "$1" == "rescue" ] || [ "$2" == "rescue" ]; then
    ADDRESS=0x2E0000
fi

TARGET=flickernoise.fbi
if [ "$2" == "compress" ] || [ "$1" == "compress" ] ; then
    TARGET=flickernoise.fbiz
fi

make -C ../src bin/${TARGET}

BATCH_FILE=`mktemp`
cat > ${BATCH_FILE}<<EOF

cable milkymist
detect
instruction CFG_OUT 000100 BYPASS
instruction CFG_IN 000101 BYPASS
pld load fjmem.bit
initbus fjmem opcode=000010
frequency 6000000
detectflash 0
endian big
flashmem ${ADDRESS} ../src/bin/${TARGET} noverify
pld reconfigure

EOF

jtag -n ${BATCH_FILE}
more    ${BATCH_FILE}
rm -f   ${BATCH_FILE}

echo "Done"
