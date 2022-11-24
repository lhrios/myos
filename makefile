.PHONY: qemu
qemu: hda.img
	qemu-system-i386 -m 1024 -k pt-br -no-reboot -cpu pentium2 \
		-drive file=hda.img,format=raw,if=ide,index=0,media=disk \
		-boot c

.PHONY: bochs
bochs: hda.img
	bochs -f bochsrc -q

.PHONY: hda.img
.ONESHELL:
hda.img: compile
	device_path=`sudo losetup --partscan --show --find hda.img`
	
	mkdir -p myos_hda0
	sudo mount $${device_path}p1 myos_hda0
	sudo cp target/bin/kernel/kernel myos_hda0/myos_kernel
	sudo cp resources/boot/regular/boot/grub/grub.cfg myos_hda0/boot/grub/grub.cfg
	sudo umount myos_hda0

	mkdir -p myos_hda1
	sudo mount $${device_path}p2 myos_hda1
	
	sudo mkdir -p myos_hda1/bin
	sudo mkdir -p myos_hda1/dev
	sudo mkdir -p myos_hda1/debug
	sudo mkdir -p myos_hda1/sbin
	sudo mkdir -p myos_hda1/tmp
	sudo ln -f -s /debug/init myos_hda1/sbin/init
	sudo ln -f -s /debug/bash myos_hda1/bin/sh
	
	sudo cp target/bin/user/executables/* myos_hda1/debug/
	sudo cp -r -L resources/hard_disk_static_files/* myos_hda1/
	
	sudo umount myos_hda1
	sudo losetup -d $$device_path	

.PHONY: sda1
.ONESHELL:
sda1: hda.img
	device_path=`sudo losetup --partscan --show --find hda.img`
	
	mkdir -p myos_hda1
	sudo mount $${device_path}p2 myos_hda1

	sudo mkdir -p /mnt/sda1/bin
	sudo mkdir -p /mnt/sda1/dev
	sudo mkdir -p /mnt/sda1/debug
	sudo mkdir -p /mnt/sda1/sbin
	sudo mkdir -p /mnt/sda1/tmp
	sudo ln -f -s /debug/init /mnt/sda1/sbin/init
	sudo ln -f -s /debug/bash /mnt/sda1/bin/sh
	
	sudo cp target/bin/kernel/kernel /mnt/sda1/
	sudo cp myos_hda1/debug/* /mnt/sda1/debug
	sudo cp -r myos_hda1/hello_world /mnt/sda1/
	sudo cp -r myos_hda1/awk /mnt/sda1/
	sudo cp -r -L resources/hard_disk_static_files/* /mnt/sda1/
	
	sudo umount myos_hda1
	sudo losetup -d $$device_path
	
compile:
	$(MAKE) -RC src

clean:
	$(MAKE) -RC src clean

fsck:
	device_path=`sudo losetup --partscan --show --find hda.img`
	sudo fsck.ext2 -f $${device_path}p2
	sudo losetup -d $$device_path
