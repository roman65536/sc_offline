# Makefile
SHELL = /bin/bash
FILES   = lua.c xlsx.c expr.c Parser.c Lexer.c sheet.c calc.c function.c session.c util.c test1.c server.c client.c html.c plugin.c slab.c
CC      = gcc
#CFLAGS  = -O6 -DNEW -g -DSYSV3 -pg -fPIC -DCOMPAT_MODULE
CFLAGS  = -g -DSYSV3 -DNEW  -fPIC -DCOMPAT_MODULE  -fvisibility=default -export-dynamic -ldl

all:	test1 server client html.so xlsx.so

test1:    Parser.o lua.o expr.o Lexer.o sheet.o calc.o function.o session.o util.o test1.o rpsc.h html.so xlsx.so
	$(CC) $(CFLAGS) lua.o  expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o test1.o plugin.c slab.c -lm `pkg-config --libs libxml-2.0 libzip lua5.2` -o test1

server:  server.c Parser.o session.o lua.o expr.o Lexer.o sheet.o calc.o function.o util.o rpsc.h slab.o
	$(CC) $(CFLAGS) slab.o lua.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o server.c -o server -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm `pkg-config --libs libxml-2.0 libzip lua5.2`

client:    client.c Parser.o lua.o expr.o Lexer.o sheet.o calc.o function.o session.o util.o rpsc.h slab.o
	$(CC) $(CFLAGS) slab.o lua.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o client.c -o client -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm `pkg-config --libs libxml-2.0 libzip lua5.2`

Lexer.c:	Lexer.l
	flex Lexer.l

Parser.c:	Parser.y Lexer.c
		bison -t Parser.y

xlsx.o:  xlsx.c
	$(CC) $(CFLAGS) -c  -D XLSX xlsx.c `pkg-config --cflags libxml-2.0`


html.so:		html.c
	gcc -g -fPIC -c html.c -o html.o `pkg-config --libs libxml-2.0 --cflags libxml-2.0` -export-dynamic
	gcc -g -fPIC -shared -rdynamic -export-dynamic  -o html.so html.o -ldl

xlsx.so:		xlsx.c
	gcc -g -fPIC -c xlsx.c -D XLSX -o xlsx.o `pkg-config --cflags libxml-2.0 libzip` -export-dynamic
	gcc -g -fPIC -shared -rdynamic -export-dynamic  -o xlsx.so xlsx.o `pkg-config --libs libxml-2.0 libzip` -ldl

%.o:            %.c rpsc.h
	$(CC) $(CFLAGS) -c $< -o $@ `pkg-config --cflags lua5.2`

clean:
	rm -f Lexer.c Parser.c Parser.h *.o gmon.out test1 server client html.so *.so
