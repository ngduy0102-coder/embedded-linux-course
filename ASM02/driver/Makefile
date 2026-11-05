KDIR := $(abspath $(CURDIR)/../../LAB01/kernel/linux-5.15)
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

KCFLAGS := -march=armv7-a -marm -msoft-float -mabi=aapcs-linux -mfpu=vfp -Wa,-march=armv7-a

obj-m += sms_sensor_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) KCFLAGS="$(KCFLAGS)" modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) clean
