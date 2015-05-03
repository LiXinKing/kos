INCLUDEDIR:= $(shell pwd)/include
CFLAGS:= -Wall -O2  -fno-builtin -nostdlib  
CPPFLAGS:= -nostdinc -I$(INCLUDEDIR)
#LIBPATH:= /home/king/workspace/cool210/crosscompile/usr/local/arm/4.4.1/lib/gcc/arm-none-linux-gnueabi/4.4.1/
export  INCLUDEDIR CFLAGS CPPFLAGS

libobj :=  lib/div64.o lib/ctype.o lib/printf.o lib/string.o lib/vsprintf.o
kernelobj := kernel/fork.o kernel/interrupt.o kernel/sched.o kernel/sys_call.o kernel/timer.o \
	kernel/trap.o kernel/uart.o kernel/mktime.o kernel/panic.o kernel/sys.o
bootobj := boot/start.o boot/lowlevel_init.o boot/cpu_init.o boot/movi.o boot/nand_cp.o 
mmobj := mm/memory.o mm/swap.o


sources  := $(bootobj)  init/main.o $(kernelobj) $(mmobj) $(libobj)

asmsource := lib/div64.o  boot/start.o boot/lowlevel_init.o boot/cpu_init.o kernel/sys_call.o

ddsource := $(filter-out $(asmsource),$(sources))

all:  boot.bin

include $(ddsource:.o=.d)

%.d: %.c
	set -e; rm -f $@; \
$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
rm -f $@.$$$$


#	-lgcc是将交叉编译器中的库函数libgcc.a编译进去，用于出发运算，否则会
#	抛出undefined reference to __aeabi_uldivmod,!!!这种尝试失败了

boot.bin:$(sources)
	arm-linux-ld -Tboot.lds -Ttext 0x00000000   -Map boot.map -o boot.elf $^
	arm-linux-objcopy -O binary boot.elf boot.bin
	arm-linux-objdump -D boot.elf > boot_elf.dis
	gcc mkv210_image.c -o mkmini210
	./mkmini210 boot.bin boot_new.bin


init/main.o:init/main.c
	arm-linux-gcc -o  $@ $< -c  $(CPPFLAGS) $(CFLAGS)
	

%.o : %.S
	arm-linux-gcc -o  $@ $< -c  $(CPPFLAGS) $(CFLAGS)
%.o : %.c
	arm-linux-gcc -o $@ $< -c   $(CPPFLAGS) $(CFLAGS)


clean:
	make clean -C lib
	make clean -C kernel
	make clean -C mm
	make clean -C boot
	rm *.o *.elf *.bin *.dis *.map mkmini210 init/*.o -f
