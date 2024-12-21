#!/bin/bash

CROSS_COMPILE_ARG=""
if [ "$CROSS_COMPILE" != "SIMULATION" ]; then
    CROSS_COMPILE_ARG="CROSS_COMPILE=mipsel-buildroot-linux-musl-"
fi

# this line deletes just the guppyscreen objects so they are all rebuilt
#docker run -ti -v $PWD:$PWD pellcorp/guppydev /bin/bash -c "cd $PWD && $CROSS_COMPILE_ARG rm build/obj/src/*"
docker run -ti -v $PWD:$PWD pellcorp/guppydev /bin/bash -c "cd $PWD && $CROSS_COMPILE_ARG make $@"
