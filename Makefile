
BINDIR = ./build/
CC = g++
INCL = include/
OBJDIR = objects/
LIBSRC  := $(wildcard library/*.h)
LIBOBJS  := $(LIBSRC:library/%.h=objects/%.o)

CFLAGS = -lraylib -O3 -std=c++17 -Wall -Wextra -Wfloat-equal -Wundef -Wshadow=compatible-local -Wpointer-arith -Winit-self
	
PHONY: example all

all: example
example: example-client example-server
	
example-client: # $(INCL)lib-net.o
	$(CC) $(CFLAGS) -o $(BINDIR)example-client  example/multiplayer_client.cpp 	
example-server: # $(INCL)lib-net.o
	$(CC) $(CFLAGS) -o $(BINDIR)example-server example/multiplayer_server.cpp 


# $(LIBOBJS): $(OBJDIR)%.o : library/%.h
# 	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)$@

# $(INCL)lib-net.o: $(LIBOBJS) 
# 	ar -rcs $(INCL)net-lib.a $(LIBOBJS) 
