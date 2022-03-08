NAME = gen-expr
SRCS = gen-expr.c
include $(NEMU_HOME)/scripts/build.mk

gen: gen-expr.c
	-gcc -g gen-expr.c -o gen-expr && ./gen-expr 200 2>/dev/null > /tmp/test.txt && echo 1 eof >> /tmp/test.txt
