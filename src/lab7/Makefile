export
CROSS_=riscv64-linux-gnu-
GCC=${CROSS_}gcc
LD=${CROSS_}ld
OBJCOPY=${CROSS_}objcopy

ISA=rv64imafd_zifencei
ABI=lp64

INCLUDE = -I $(shell pwd)/include -I $(shell pwd)/arch/riscv/include
CF = -march=$(ISA) -mabi=$(ABI) -mcmodel=medany -fno-builtin -ffunction-sections -fdata-sections -nostartfiles -nostdlib -nostdinc -static -lgcc -Wl,--nmagic -Wl,--gc-sections -g 
# DEFINE = -D SJF
DEFINE = -D PRIORITY_SCHEDULE
CFLAG = ${CF} ${INCLUDE} ${DEFINE}

.PHONY:all run debug clean
all: clean
	${MAKE} -C user all
	${MAKE} -C lib all
	${MAKE} -C init all
	${MAKE} -C test all
	${MAKE} -C fs all
	${MAKE} -C arch/riscv all
	@echo -e '\n'Build Finished OK

debug: all
	@echo Launch the qemu ......
	@qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios default \
		-kernel vmlinux \
		-global virtio-mmio.force-legacy=false \
		-drive file=disk.img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0 \
		-S -s

run: all
	@echo Launch the qemu ......
	@qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios default \
		-kernel vmlinux \
		-global virtio-mmio.force-legacy=false \
		-drive file=disk.img,if=none,format=raw,id=hd0 \
		-device virtio-blk-device,drive=hd0

TEST:
	${MAKE} -C lib all
	${MAKE} -C test test
	${MAKE} -C init all
	${MAKE} -C arch/riscv all
	@echo -e '\n'Build Finished OK


test-run: TEST
	@echo Launch the qemu ......
	@qemu-system-riscv64 -nographic -machine virt -kernel vmlinux -bios default 



test-debug: TEST
	@echo Launch the qemu for debug ......
	@qemu-system-riscv64 -nographic -machine virt -kernel vmlinux -bios default -S -s

clean:
	${MAKE} -C lib clean
	${MAKE} -C user clean
	${MAKE} -C fs clean
	${MAKE} -C test clean
	${MAKE} -C init clean
	${MAKE} -C arch/riscv clean
	$(shell test -f vmlinux && rm vmlinux)
	$(shell test -f System.map && rm System.map)
	@echo -e '\n'Clean Finished
	
compile_commands.json: clean
	bear -- make
	sed -i 's/_zifencei//g' compile_commands.json
