#!/usr/bin/env bash

cp aio-switch-updater-reborn.elf release-aio-switch-updater-reborn.elf

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR
rm -r switch/aio-switch-updater-reborn/
mkdir -p switch/aio-switch-updater-reborn/
cp aio-switch-updater-reborn.nro switch/aio-switch-updater-reborn/
#VERSION=$(grep "APP_VERSION :=" Makefile | cut -d' ' -f4)
#cp aiosu-forwarder/aiosu-forwarder.nro switch/aio-switch-updater-reborn/aio-switch-updater-reborn-v$VERSION.nro
zip -FSr aio-switch-updater-reborn.zip switch/