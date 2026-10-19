
obj-m	+=w25q.o

all drivers:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C  /lib/modules/$(shell uname -r)/build M=$(PWD) clean
