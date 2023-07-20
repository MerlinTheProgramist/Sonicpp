all: client server	

client:
	g++ -o client multiplayer_client.cpp include/libraylib.so.4.2.0	
server:
	g++ multiplayer_server.cpp -o server
