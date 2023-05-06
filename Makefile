CC := gcc
CFLAGS := -O0 -g

TARGET := buddy

.PHONY: all
all: $(TARGET)

$(TARGET): main.o buddy.o

.PHONY: clean
clean:
	rm -rf $(TARGET) *.o
