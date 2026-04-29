CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS ?= -lws2_32
TARGET = drinks_c.exe
SRC = backend/src/server.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /Q $(TARGET) 2>NUL || exit 0

run: $(TARGET)
	$(TARGET)
