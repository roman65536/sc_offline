SHELL = /bin/bash
CC	= gcc
CFLAGS	= `xmlrpc-c-config  abyss-server --cflags` -g -DSYSV3 -DNEW  -fPIC -DCOMPAT_MODULE  -fvisibility=default -export-dynamic  -I../
LIBS_s	= `xmlrpc-c-config  abyss-server --libs` -ldl
LIBS_c	= `xmlrpc-c-config  client --libs` -ldl
OBJ 	= ../calc.o  ../expr.o  ../function.o  ../Lexer.o  ../Parser.o  ../session.o  ../sheet.o  ../util.o   ../slab.o ../plugin.o



all:	server client


client:		xmlrpc_client.o  
	$(CC) $(LIBS_c) -o xml_client $< 

server:		xmlrpc_server.o  
	$(CC) $(LIBS_s) -o xml_server $< $(OBJ) -lm -ldl


%.o:            %.c ../rpsc.h
	$(CC) $(CFLAGS) -c $< -o $@  


