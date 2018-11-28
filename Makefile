# Makefile
SHELL = /bin/bash
FILES   = lua.c xlsx.c expr.c Parser.c Lexer.c sheet.c calc.c function.c session.c util.c test1.c server.c client.c html.c plugin.c slab.c tui/*.c
SC_LIB_C   = lua.c xlsx.c expr.c Parser.c Lexer.c sheet.c calc.c function.c session.c util.c html.c plugin.c slab.c
#SC_LIB_O   = lua.o xlsx.o expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o html.o plugin.o slab.o
SC_LIB_O   = expr.o Parser.o Lexer.o sheet.o calc.o function.o session.o util.o plugin.o slab.o
CC      = gcc
#CFLAGS  = -O6 -DNEW -g -DSYSV3 -pg -fPIC -DCOMPAT_MODULE
CFLAGS  = -pg -g -DSYSV3 -DNEW  -fPIC -DCOMPAT_MODULE  -fvisibility=default -export-dynamic -ldl -lpthread -D_XOPEN_SOURCE_EXTENDED

all:	html.so xlsx.so test1 server client tui


sc_lib: $(SC_LIB_O)
	ar -cvr libsc_s.a  $(SC_LIB_O)

test1:  sc_lib test1.o rpsc.h html.so xlsx.so
	$(CC) $(CFLAGS) test1.o -L./ -lsc_s -lm `pkg-config --libs libxml-2.0 libzip lua5.2` -o test1


lua:	$(SC_LIB_O) lua.o
#	$(CC) $(CFLAGS) lua.o -o sc.so -shared -L./ -lsc -lm `pkg-config --libs libxml-2.0  libzip lua5.2` -L$(HOME)/Downloads/libxls-master/.libs -lxlsreader
	$(CC) $(CFLAGS) -Wl,--whole-archive  libsc.a -Wl,--no-whole-archive lua.o -Wl,-soname,sc.so  -Wl,--version-script=sc.version -o sc.so -fvisibility=default -shared -rdynamic -export-dynamic -L./   -lm `pkg-config --libs libxml-2.0  libzip lua5.2` -L$(HOME)/Downloads/libxls-master/.libs  -ldl
#	$(CC) $(CFLAGS) lua.o $(SC_LIB_O) -Wl,-soname,sc.so  -o sc.so -fvisibility=default -shared -rdynamic -export-dynamic -L./   -lm `pkg-config --libs libxml-2.0  libzip lua5.2` -L$(HOME)/Downloads/libxls-master/.libs  -ldl

server: sc_lib server.o rpsc.h
	$(CC) $(CFLAGS) server.o -L./ -lsc -o server -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0 -lm

client: client.o
	$(CC) $(CFLAGS) client.o -L./ -o client -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0

tui: tui.o
	$(CC) $(CFLAGS) tui.o -L./ -o tui/tui -lncursesw -lmsgpackc -Lmsgpack-c/libmsgpackc.so.2.0.0

Lexer.c:	Lexer.l
	flex Lexer.l

Parser.c:	Parser.y Lexer.c
		bison -t Parser.y

xlsx.o:  xlsx.c
	$(CC) $(CFLAGS) -c  -D XLSX xlsx.c `pkg-config --cflags libxml-2.0`


html.so:		html.c
	gcc -pg -g -fPIC -c html.c -o html.o `pkg-config --libs libxml-2.0 --cflags libxml-2.0` -export-dynamic
	gcc -pg -g -fPIC -shared -rdynamic -export-dynamic  -o html.so html.o -ldl

xlsx.so:		xlsx.c
	gcc -pg -g -fPIC -c xlsx.c -D XLSX -o xlsx.o `pkg-config --cflags libxml-2.0 libzip` -export-dynamic -fvisibility=default
	gcc -pg -g -fPIC -shared -rdynamic -export-dynamic  -o xlsx.so xlsx.o `pkg-config --libs libxml-2.0 libzip ` -L./ -lsc  -ldl

%.o:            %.c rpsc.h
	$(CC) $(CFLAGS) -fPIC -c $< -o $@ `pkg-config --cflags lua5.2` -export-dynamic

tui.o:           tui/tui.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Lexer.c Lexer.h Parser.c Parser.h *.o gmon.out test1 server client html.so *.so *.a tui/*.o tui/tui
