CFLAGS+=-O3 -Wall -Wextra -Wpedantic

coresched: coresched.o

.PHONY: clean
clean:
	$(RM) coresched coresched.o
