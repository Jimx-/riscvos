#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

RISCVPK=$DIR/riscv-pk

export PATH=$RISCV/bin:$PATH

if [ ! -d "$RISCVPK/build" ]; then
    mkdir $RISCVPK/build
fi

pushd $RISCVPK/build > /dev/null

../configure --prefix=$DIR/toolchain/ --host=riscv64-unknown-elf --with-payload=$DIR/obj/kernel
make

popd > /dev/null

cp $DIR/riscv-pk/build/bbl $DIR
echo $DIR
