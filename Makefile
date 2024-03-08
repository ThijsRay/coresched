CFLAGS+=-O3 -Wall -Wextra -Wpedantic -g

coresched: coresched.o

.PHONY: clean
clean:
	$(RM) coresched coresched.o
