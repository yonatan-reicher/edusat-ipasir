NAME = edusat
TAR_NAME = $(NAME).tar.gz
SRC_DIR = src
SRC_CPP = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/*/*.cpp)
DIR = $(NAME)
TARGET = libipasir$(NAME).a

CC = g++
CFLAGS = -std=c++17 -DNDEBUG -O3

all: $(TARGET)

$(TARGET): .FORCE extract build

extract:
	tar zxf $(TAR_NAME) --overwrite

pack:
	echo 'g++' > LINK
	echo '-lm -lz -lstdc++' > LIBS
	tar zcf $(TAR_NAME) $(SRC_DIR) LINK LIBS makefile
	mkdir -p ipasir/sat/$(NAME)
	mv $(TAR_NAME) ipasir/sat/$(NAME)/
	cp makefile ipasir/sat/$(NAME)/
	rm -f LINK LIBS


build:
	$(CC) $(CFLAGS) -c $(SRC_CPP)
	ar r $(TARGET) *.o
	rm -f *.o

test:
	$(CC) -g test.cpp $(SRC_CPP) -o test.out
	./test.out
	rm test.out

.FORCE:
.PHONY: all clean extract build pack test
