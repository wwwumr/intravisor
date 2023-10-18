PROG=helloworld
PROG_C=$(PROG).c

MOUNTPOINT=/media/ext4disk

TARGET=${HOME}/cheri/output/intravisor

DISK=disk_hello.img

LOOP_DEVICE=loop9
IMAGE_SIZE_KB=400

ESCALATE_CMD=sudo

.DELETE_ON_ERROR:
.PHONY: all clean

all: $(DISK)

install:
	cp *.img *.yaml ${TARGET}

clean:
	rm -f $(DISK) $(PROG) *.o

$(PROG): $(PROG_C)
	../../build/musl-host/bin/musl-clang-target -fPIE -pie -c -g -o $@.o $(PROG_C)
	../../build/musl-host/bin/ld.musl-clang-target -g -pie -dynamic-linker /ld.so helloworld.o -o helloworld

$(DISK): $(PROG) 
	dd if=/dev/zero of="$@" count=$(IMAGE_SIZE_KB) bs=1K
	mkfs.ext4 "$@"
	$(ESCALATE_CMD) /bin/bash -euxo pipefail -c '\
		mkdir -p $(MOUNTPOINT); \
		mount -t ext4 -o loop "$@" $(MOUNTPOINT); \
		mkdir -p $(MOUNTPOINT)/app; \
		echo "Hello World!" > $(MOUNTPOINT)/app/helloworld.txt; \
		cp $(PROG)  $(MOUNTPOINT)/app; \
		umount $(MOUNTPOINT); \
		chown $(USER) "$@"; \
	'

test: $(DISK)
	../../build/sgx-lkl-run $(DISK) app/$(PROG)

