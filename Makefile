# Makefile
SHELL = /bin/bash
FILES   = lua.c xlsx.c expr.c Parser.c Lexer.c sheet.c calc.c function.c session.c util.c test1.c server.c client.c
CC      = gcc
CFLAGS  = -O6 -DNEW -g -DSYSV3 -pg -fPIC -DCOMPAT_MODULE

test1:    lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o test1.o rpsc.h
	$(CC) $(CFLAGS) lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o test1.o -lm `pkg-config --libs libxml-2.0 libzip lua5.2` -o test1

server:    session.o lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o util.o rpsc.h
	$(CC) $(CFLAGS) lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o server.c -o server -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm `pkg-config --libs libxml-2.0 libzip lua5.2`
#$(CC) $(CFLAGS) lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o server.c -static -o server -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm `pkg-config --libs libxml-2.0 libzip lua5.2`

client:    lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o rpsc.h
	$(CC) $(CFLAGS) client.c -o client -lm `pkg-config --libs libxml-2.0 libzip lua5.2`

Lexer.c:	Lexer.l
	flex Lexer.l

Parser.c:	Parser.y Lexer.c
		bison -t Parser.y

xlsx.o:         xlsx.c
	$(CC) $(CFLAGS) -c  -D XLSX xlsx.c `pkg-config --cflags libxml-2.0`

%.o:            %.c rpsc.h
	$(CC) $(CFLAGS) -c $< -o $@ `pkg-config --cflags lua5.2`

clean:
	rm -f Lexer.c Parser.c Parser.h *.o gmon.out test1 server client
