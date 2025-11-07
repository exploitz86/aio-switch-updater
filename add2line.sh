#/bin/bash
sed -nr 's|^\s*(\S+):.*\(\S+ \+ (0x[0-9a-f]+)\).*$|echo -n \1 : ; aarch64-none-elf-addr2line -e release-aio-switch-updater-reborn.elf -pCf -si \2|ep' $1