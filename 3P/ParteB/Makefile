obj-m +=  pcmod2.o 
pcmod2-objs += cbuffer.o fifoproc.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

