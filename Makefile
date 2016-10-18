#!/usr/bin/make -f
# Makefile for confconf program.

TARGET = confconf
OBJECTS = $(TARGET).o

CC = gcc
CFLAGS = -std=c99 -g -Wall

default: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)

install: 
	cp $(TARGET) /usr/bin/$(TARGET)
