#!/bin/bash

PRINTER_IP=
SETUP=false

GIT_REVISION=$(git rev-parse --short HEAD)
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD | awk -F '/' '{print $2}')

function docker_make() {
    docker run -ti -v $PWD:$PWD pellcorp/guppydev /bin/bash -c "cd $PWD && GUPPYSCREEN_VERSION=$GIT_REVISION GUPPYSCREEN_BRANCH=$GIT_BRANCH CROSS_COMPILE=mipsel-buildroot-linux-musl- make $@"
}

while true; do
    if [ "$1" = "--setup" ]; then
        SETUP=true
        shift
    elif [ "$1" = "--printer" ]; then
        shift
        PRINTER_IP=$1
        shift
    else
        break
    fi
done

if [ "$SETUP" = "true" ]; then
    docker_make spdlogclean || exit $?
    docker_make libhvclean || exit $?
    docker_make wpaclean || exit $?
    docker_make clean || exit $?

    docker_make libhv.a || exit $?
    docker_make wpaclient || exit $?
    docker_make libspdlog.a || exit $?
else
    docker_make $1 || exit $?

    if [ -n "$PRINTER_IP" ] && [ -f build/bin/guppyscreen ]; then
        sshpass -p 'creality_2023' scp build/bin/guppyscreen root@$PRINTER_IP:
        sshpass -p 'creality_2023' ssh root@$PRINTER_IP "mv /root/guppyscreen /usr/data/guppyscreen"
        sshpass -p 'creality_2023' ssh root@$PRINTER_IP "/etc/init.d/S99guppyscreen restart"
    fi
fi
