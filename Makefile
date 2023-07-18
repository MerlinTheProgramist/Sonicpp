all: client server	

client:
	g++ multiplayer_client.cpp -o client
server:
	g++ multiplayer_server.cpp -o server
