#!/bin/bash

set -e

if [[ -f "hda.img" ]]; then
	echo "File \"hda.img\" already exists. Skipping..."
	exit 0
fi

qemu-img create -f raw hda.img 1G
sfdisk hda.img << EOF
label: dos
unit: sectors
/dev/loop21p1 : start=        2048, size=      262144, type=c
/dev/loop21p2 : start=      264192, size=     1832960, type=83
EOF
device_path=`sudo losetup --partscan --show --find hda.img`

sudo mkfs.fat "${device_path}p1"
sudo mkfs.ext2 "${device_path}p2" -O sparse_super,large_file

mkdir -p "/tmp/myos_hda0$$"
sudo mount "${device_path}p1" "/tmp/myos_hda0$$"
sudo grub-install --boot-directory="/tmp/myos_hda0$$/boot" $device_path
sudo umount "${device_path}p1"

sudo losetup -d $device_path
