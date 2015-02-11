# You can put your build options here
-include config.mk

all: jsondump

libjstn.a: jstn.o
	$(AR) rc $@ $^

%.o: %.c jstn.h
	$(CC) -c $(CFLAGS) $< -o $@

jsondump: example/jsondump.o libjstn.a
	$(CC) $(LDFLAGS) $^ -o $@

clean:
	rm -f libjstn.a
	rm -f example/jsondump.o
	rm -f jsondump jsondump.exe

.PHONY: all clean
