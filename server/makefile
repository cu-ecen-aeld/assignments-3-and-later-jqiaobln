CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?=	-g	-Wall -pthread

TARGET ?=aesdsocket
LDFLAGS ?= -lpthred -lrt

all:	$(TARGET)

$(TARGET):	$(TARGET)
	$(CC)	$(CFLAGS) -o $(TARGET)	$(TARGET).c

clean:
	rm -f *.o $(TARGET)