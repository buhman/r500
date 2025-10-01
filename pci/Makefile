BINARY := test_pci_module
KERNEL := /lib/modules/$(shell uname -r)/build
ARCH := x86
C_FLAGS := -Wall
KMOD_DIR := $(shell pwd)

OBJECTS := main.o

ccflags-y += $(C_FLAGS)

obj-m += $(BINARY).o

$(BINARY)-y := $(OBJECTS)

$(BINARY).ko:
	make -C $(KERNEL) M=$(KMOD_DIR) modules

clean:
	rm -f $(BINARY).ko
