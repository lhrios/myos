#!/bin/bash

set -e

DISK_IMAGE_FILE_PATH="hda_integration_tests.img"
integration_tests_root_path="/tmp/$$root"
integration_tests_boot_path="/tmp/$$boot"

mkdir -p $integration_tests_root_path
mkdir -p $integration_tests_boot_path

make compile

device_path=`sudo losetup --partscan --show --find $DISK_IMAGE_FILE_PATH`

sudo mount $device_path"p1" $integration_tests_boot_path
sudo cp target/bin/kernel/kernel $integration_tests_boot_path/myos_kernel
sudo cp resources/boot/integration_tests/boot/grub/grub.cfg $integration_tests_boot_path/boot/grub/grub.cfg
sudo umount $integration_tests_boot_path

sudo mkfs.ext2 -t ext2 -O sparse_super,large_file -F -F -q $device_path"p2"
sudo mount $device_path"p2" $integration_tests_root_path

pushd $integration_tests_root_path
sudo mkdir -p dev
sudo mkdir -p sbin
sudo mkdir -p executables
sudo mkdir -p tmp
sudo ln -s /executables/test_suite sbin/init
popd

sudo cp -r target/bin/test/integration/executables/* "$integration_tests_root_path/executables"
sudo mkdir -p "$integration_tests_root_path/resources"
sudo cp -r resources/test/integration/* "$integration_tests_root_path/resources"

#pushd $integration_tests_root_path
#sudo tar czf /tmp/myos_integration_tests.tar.gz *
#popd

sudo umount $integration_tests_root_path
sudo losetup -d $device_path

qemu-system-i386 -m 2048 -k pt-br -no-reboot -cpu pentium2 \
	-drive file=$DISK_IMAGE_FILE_PATH,format=raw,if=ide,index=0,media=disk \
	-boot c

#bochs -f bochsrc.integration_tests -q

device_path=`sudo losetup --partscan --show --find $DISK_IMAGE_FILE_PATH`
sudo fsck.ext2 -n -f $device_path"p2"
if [ $? == 0 ]; then
	sudo mount $device_path"p2" $integration_tests_root_path
	
	#sudo find $integration_tests_root_path -iname '*seek*.tmp' -exec cat {} + 
	
	#sudo find $integration_tests_root_path

	sudo ./src/test/integration/compute_integration_tests_summary.py $integration_tests_root_path/executables \
		$integration_tests_root_path/integration_test_output
	
	sudo umount $integration_tests_root_path
else
	printf "\x1B[91mERROR: File system was not clean!\x1B[m\n"
fi

sudo losetup -d $device_path

rmdir $integration_tests_root_path
rmdir $integration_tests_boot_path
