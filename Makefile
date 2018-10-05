# Makefile
SHELL = /bin/bash
FILES   = lua.c xlsx.c expr.c Parser.c Lexer.c sheet.c calc.c function.c session.c util.c test1.c server.c client.c html.c plugin.c slab.c
CC      = gcc
#CFLAGS  = -O6 -DNEW -g -DSYSV3 -pg -fPIC -DCOMPAT_MODULE
CFLAGS  = -g -DSYSV3 -DNEW  -fPIC -DCOMPAT_MODULE  -fvisibility=default -export-dynamic -ldl

test1:    lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o test1.o rpsc.h
	$(CC) $(CFLAGS) lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o test1.o html.c plugin.c slab.c -lm `pkg-config --libs libxml-2.0 libzip lua5.2` -o test1

server:    session.o lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o util.o rpsc.h slab.o
	$(CC) $(CFLAGS) lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o server.c slab.c -o server -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm `pkg-config --libs libxml-2.0 libzip lua5.2`

client:    lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o rpsc.h slab.o
	$(CC) $(CFLAGS) lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o client.c slab.c -o client -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm `pkg-config --libs libxml-2.0 libzip lua5.2`

Lexer.c:	Lexer.l
	flex Lexer.l

Parser.c:	Parser.y Lexer.c
		bison -t Parser.y

xlsx.o:         xlsx.c
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
	rm -f Lexer.c Parser.c Parser.h *.o gmon.out test1 server client
