
TARGET=PCU100
CC=C:\BCRAFT\c6805.exe
OPTIONS=+ds +l +e +x

HDRS=			\
system.h        \
kernel.h		\
timer.h			\
ledstat.h		\
spucomm.h		\
pcumngr.h		\
motor.h			\
atod.h			\
digin.h			\
digout.h		\
updownsw.h		\
shcountr.h


SRCS=			\
kernel.c		\
timer.c			\
ledstat.c		\
spucomm.c		\
pcumngr.c		\
motor.c			\
atod.c			\
digin.c			\
digout.c		\
updownsw.c		\
shcountr.c		\
$(TARGET).c

$(TARGET).cod: makefile.mak $(HDRS) $(SRCS)
	$(CC) $(OPTIONS) $(TARGET).c


