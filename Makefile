CFLAGS+=-O3 -Wall -Wextra -Wpedantic -g

coresched: coresched.o

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: install
install: coresched
	install -m 755 $< -D "$(PREFIX)/bin/"
	setcap CAP_SYS_PTRACE+ep $<


.PHONY: clean
clean:
	$(RM) coresched coresched.o
