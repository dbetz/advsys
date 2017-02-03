CC?=propeller-elf-gcc

CFLAGS=-Wall -Os

ifeq ($(CC),cc)
EXT=
endif

ifeq ($(CC),propeller-elf-gcc)
CFLAGS += -mcmm
EXT=.elf
endif

COBJS=\
advcom.o \
advavl.o \
advcio.o \
advexp.o \
advfcn.o \
advscn.o

IOBJS=\
advexe.o \
advdbs.o \
adviio.o \
advint.o \
advmsg.o \
advprs.o \
advtrm.o

all:	advcom$(EXT) advint$(EXT)

advcom$(EXT):	$(COBJS)
	$(CC) $(CFLAGS) -o $@ $(COBJS)

advint$(EXT):	$(IOBJS)
	$(CC) $(CFLAGS) -o $@ $(IOBJS)

%.o:	%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o advcom$(EXT) advint$(EXT) *.dat
