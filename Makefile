
BINDIR = ./build/
CC = g++
INCL = include/
OBJDIR = objects/
LIBSRC  := $(wildcard library/*.h)
LIBOBJS := $(LIBSRC:library/%.h=objects/%.o)

CFLAGS = -ggdb -std=c++20 -fext-numeric-literals -Wall -Wextra -Wfloat-equal -Wundef -Wshadow=compatible-local -Wpointer-arith -Winit-self
	
PHONY: example all

all: examples
examples: example-ping example-raylib example-tictactoe


example-ping:
	@$(CC) $(CFLAGS) -o $(BINDIR)$@-client examples/ping_server/simpleClient.cpp 	
	@$(CC) $(CFLAGS) -o $(BINDIR)$@-server examples/ping_server/sampleServer.cpp 	

example-tictactoe:
	@$(CC) $(CFLAGS) -o $(BINDIR)$@-client examples/tic_tac_toe/client.cpp 	
	@$(CC) $(CFLAGS) -o $(BINDIR)$@-server examples/tic_tac_toe/server.cpp 	

example-raylib:
	@$(CC) $(CFLAGS) -lraylib -o $(BINDIR)$@-client examples/raylib_2d_example/multiplayer_client.cpp 	
	@$(CC) $(CFLAGS) -lraylib -o $(BINDIR)$@-server examples/raylib_2d_example/multiplayer_server.cpp 


# $(LIBOBJS): $(OBJDIR)%.o : library/%.h
# 	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)$@

# $(INCL)lib-net.o: $(LIBOBJS) 
# 	ar -rcs $(INCL)net-lib.a $(LIBOBJS) 
