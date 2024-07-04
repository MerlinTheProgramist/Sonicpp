This is a simple UDP network library for multiplayer games.

Could be used for self hosting by starting a server instance with the client and connectin to it

# Get Started
1. Decide what types of data you will be sending
```cpp
// common.h
enum class MyGameMsg : uint32_t{
  // general message types 
  PlayerMove,
  ClientAccepted,
  // ...
  
  // message types from clients
  Client_PlayerShoot,
  // ...
  
  // message types from server
  Server_PlayerKilled
  // ...
};
```
2. Design a server class
```cpp
// server.cpp
#include "common.h"
class MyServer : public net_frame::ServerInterface<GameMsg>
{
  int player_next_id{0};
public:    
  MyServer(uint16_t port)
  : net_frame::ServerInterface<GameMsg>(port)
  {
    Start();
      
    for(;;)
    {
      // unlimited number of capacitors
      Update(-1, true);
    }
  }
protected:
  // REQUIRED TO PROVIDE
  /// return true if new client can join
  bool OnClientConnect(std::shared_ptr<Connection> client)
  override
  {
    player_counter ++;
    std::cout << "[SERVER] New player joined the lobby!" << std::endl;
    return true;
  }

  // REQUIRED TO PROVIDE
  /// setup game state for the new player 
  void OnClientValidated(std::shared_ptr<Connection> client) 
  override
  {
    // Send a custom message that tells the client he, can now communicate
    Message msg{GameMsg::Client_Accepted};
    msg << player_next_id++;
    client->Send(msg);
  }

  // REQUIRED TO PROVIDE
  /// track which players quit the game and tell other players about this event
  void OnClientDisconnect(std::shared_ptr<Connection> client) 
  override
  {
    std::cout << "[UNGRECEFULL REMOVAL]: " << client->GetID() << std::endl;

    Message m{MyGameMsg::Server_RemovePlayer};
    m << client->GetID();
    MessageAllClients(m);
  }

  // REQUIRED TO PROVIDE
  /// handle new message from client
  void OnMessage(std::shared_ptr<Connection> client, Message& msg) 
  override
  {
      switch(msg.GetType()){
        case MyGameMsg::PlayerMove:
        {
          // ... update game state
          // send to other clients, except the origin
          MessageAllClients(msg, client);
        }
      }
  }
};
```

3. Design a client class
```cpp
// client.cpp
#include "common.h"
class MyClient : public net_frame::ClientInterface
{
int player_id{};
bool accepted{false};
public:
  MyClient(std::string& ip, uint16_t port)
  :ClientInterace{}
  ,server_instance
  {
    if(!Connect(ip, port)){
      std::cerr << "Failed to connect!" << std::endl;
      exit(1);
    }

    while(/* wait for next frame */)
    {
      Update();
    }
  }

  void Update()
  {
    // receive messages
    if(IsConnected()){
      while(auto msg = NextMessage()){
        switch msg->GetType(){
          case GameMsg::Client_Accepted:
          {
              std::cout << "Server Accepted you!" << std::endl;

              player_id << *msg;
              accepted = true;
          }
          break;
          case MyGameMsg::PlayerMove:
          {
            std::pair<int,int> pos;
            pos << *msg;
            // ... update your local game state 
          }
          break;
          // ... other msg types send by server

          default:
            std::cerr << "Not suppoerted message type received" << std::endl
        }
      }
    }
  
    // ... game logic

    if(accepted){
      // sending data to server
      Message my_message{MyGameMsg::PlayerMove};
      // optionaly provide any flat objects or vectors
      my_message << std::pair<int,int>(0,0) << player_id;

      // send assembled message to server
      Send(my_message);
    }
    
    // ... rendering
  }
};
int main(int argc, char* argv[])
{
  std::string host_ip{(argc>=2)?argv[1]:"127.0.0.1"};
  uint16_t port = 1234;
  MyClient my_client{host_ip, host_ip};
}
```
# Example
Some provided examples require raylib and raylib-cpp to be compiled

