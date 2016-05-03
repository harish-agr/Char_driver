obj-m := char_driver_skelton.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
	gcc -o userapp userapp.c
	gcc -o ioctl ioctl_entry_test.c

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
	rm -rf userapp
	rm -rf ioctl
