all: dgsh-compat hello

hello: hello.c dgsh-elf.s

dgsh-compat: dgsh_util.c
	cc -o $@ -DDGSH_COMPAT $?

test:
	./dgsh-compat hello && echo OK
