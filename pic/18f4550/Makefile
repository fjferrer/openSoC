# MakeFile para compilar el firmware v1.0

CHIP=4550
SDCC_HOME=/usr/share/sdcc/
HEXFILE=USB.hex

CC=sdcc

CFLAGS= -Wl --vc  --denable-peeps  --optimize-cmp --optimize-df\
--obanksel=2 --opt-code-size --fommit-frame-pointer -mpic16 -p18f4550 -I\
$(SDCC_HOME)\include/pic16/ -c

LDFLAGS= --vc --denable-peeps  --optimize-cmp --optimize-df\
--obanksel=2 --opt-code-size --fommit-frame-pointer -mpic16 -p18f4550\
-L $(SDCC_HOME)\lib/pic16/ -Wl,"-w -s 18f$(CHIP).lkr"

###########################################################################

all: main.c usb.h usb.o
	$(CC) $(LDFLAGS)  main.c usb.o

usb.o: usb.c usb.h 
	$(CC) $(CFLAGS) usb.c

clean:
	rm *.asm
	rm *.lst
	rm *.o
	rm *.cod
delete:
	rm *.hex
