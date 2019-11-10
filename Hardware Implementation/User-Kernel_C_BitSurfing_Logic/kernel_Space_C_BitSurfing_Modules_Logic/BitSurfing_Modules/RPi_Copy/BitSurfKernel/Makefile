TARGET = bitReceiver

obj-m += $(TARGET).o

$(TARGET)-objs := rec.o kfile.o kgetline.o

EXTRA_CFLAGS := -I$(PWD)

KDIR = /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

test:
	dmesg -C
	insmod bitReceiver.ko
	rmmod bitReceiver.ko
	dmesg
