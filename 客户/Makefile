
TOPDIR  	:= $(shell pwd)
INCDIR 		:= $(TOPDIR)/inc
SRCDIR  	:= $(TOPDIR)/src

CPPFLAGS	:= -I$(INCDIR)
CFLAGS		:= -fno-builtin -Wall -O2

export 	TOPDIR INCDIR SRCDIR  CPPFLAGS CFLAGS

TARGET = x86
TARGET_ARM = aicProject
SRCS	:= $(wildcard *.c)
SRCS	+= $(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.c))

OBJS	:= $(patsubst %.c,%.o,$(SRCS))
ARM_OBJ :=$(patsubst %.c,%.arm.o,$(SRCS))

TSFLAGS   =  -L/usr/local/tslib/lib -lts
PICFLAGS  =  -L/var/jpeg-lib/lib -ljpeg 
PICFLAGS  =  -ljpeg 
THRFLAGS  =  -lpthread
MYSQLCLIENT = -lmysqlclient

all:$(TARGET) $(TARGET_ARM)

gcc:$(TARGET)

arm:$(TARGET_ARM)


$(TARGET):$(OBJS)
	gcc  $(OBJS) -o $(TARGET)  $(PICFLAGS) $(THRFLAGS) 
	rm -rf *.o
	rm -rf $(SRCDIR)/*.o
$(OBJS):%.o:%.c
	gcc  -c  $^ -o  $@  $(CPPFLAGS)  $(CFLAGS)  
	
$(TARGET_ARM):$(ARM_OBJ) 
	arm-linux-gcc  $(ARM_OBJ) -o $(TARGET_ARM)  $(TSFLAGS) $(PICFLAGS) $(THRFLAGS) 
	cp aicProject /nfs
	rm -rf *.o
	rm -rf $(SRCDIR)/*.o
%.arm.o:%.c
	arm-linux-gcc  -c $^ -o $@ $(CPPFLAGS) $(CFLAGS) 

