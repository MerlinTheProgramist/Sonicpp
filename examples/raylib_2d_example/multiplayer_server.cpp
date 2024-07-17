#include <unordered_map>
#include <iostream>

#include "../../library/server.h"
#include "multiplayer_common.hpp"


class GameServer : public sonicpp::ServerInterface<GameMsg>
{
public:
  GameServer(uint16_t port):sonicpp::ServerInterface<GameMsg>(port)
  {
    Start();

    while(1)
    {
      Update(-1, true);
    }
    
  }  

  std::unordered_map<uint32_t, PlayerDescription> clientRoster;
  std::vector<uint32_t> GarbageIDs;
  
protected:
  bool OnClientConnect(std::shared_ptr<Connection> client)
  override
  {
    return true;
  }

  void OnClientValidated(std::shared_ptr<Connection> client) 
  override
  {
    // Send a custom message that tells the client he, can now communicate
    Message msg;
    msg.header.id = GameMsg::Client_Accepted;
    client->Send(msg);
  }

  void OnClientDisconnect(std::shared_ptr<Connection> client) 
  override
  {
    if(client)
    {
      if(clientRoster.find(client->GetID()) == clientRoster.end())
      {
        // Client never has been added to game
        
      }
      else
      {
        auto& pd = clientRoster[client->GetID()];
        std::cout << "[UNGRECEFULL REMOVAL]: " << pd.uUniqueID << std::endl;
        clientRoster.erase(client->GetID());
        GarbageIDs.push_back(client->GetID());
      }
    }
  }

  void OnMessage(std::shared_ptr<Connection> client, Message& msg) 
  override
  {
    // If there are some clients that had disconnected
    // remind all clients that a client had disconnected
    if(!GarbageIDs.empty())
    {
      for(auto pid : GarbageIDs)
      {
        Message m;
        m.header.id = GameMsg::Game_RemovePlayer;
        m << pid;
        std:: cout << "Removing: " << pid << std::endl;
        MessageAllClients(m);
      }
      GarbageIDs.clear();        
    }
    
    switch(msg.GetType())
    {
      case GameMsg::Client_RegisterWithServer:
      {
          // Receive player data
          PlayerDescription desc;
          {
          msg >> desc;
          desc.uUniqueID = client->GetID();
          clientRoster[desc.uUniqueID] = desc;
          }
          
          // Now send to playar its ID
          {
          Message msgSendID;
          msgSendID.header.id = GameMsg::Client_AssignID;
          msgSendID << desc.uUniqueID;
          MessageClient(client, msgSendID);
          }
          {
          // Inform players that a new player has joined
          Message msgAddPlayer;
          msgAddPlayer.header.id = GameMsg::Game_AddPlayer;
          msgAddPlayer << desc;
          MessageAllClients(msgAddPlayer, client);
          }

          // send all other client informations to the client that just joined
          for(const auto& player : clientRoster)
          {
            Message msgAddOtherPlayers{};
            msgAddOtherPlayers.header.id = GameMsg::Game_AddPlayer;
            msgAddOtherPlayers << player.second;
            MessageClient(client, msgAddOtherPlayers);
          }
      }
      break;
      case GameMsg::Client_UnregisterWithServer:
      {
          
      }
      break;
      case GameMsg::Game_UpdatePlayer:
      {
          // @FOR_NOW check collisions every time a packet is received
          // UpdatePlayerCollisions(playerRoster[client->GetID()], playerRoster,
          //   [&](idT affected){
          //     message collision;
          //     collision.header.id = GameMsg::Game_UpdatePlayer;
          //     collision << playerRoster[affected];
          //     MessageAllClients(collision,client);
          //   });
          // and compose a new message

          
          // send to all other players except the sender
          MessageAllClients(msg, client);
      }
      break;
      case GameMsg::Game_UpdatePlayerLook:
        MessageAllClients(msg, client);
      break;
      default:
      break;
    }
  }

};


int main(int argc, char* argv[])
{
  uint16_t port = 60'000;
  if(argc == 1)
  {
    std::cout << "[INFO] using default port: " << port << std::endl;
  }
  if(argc == 2)
  {
    port = std::stoi(argv[1]); 
  }

  
  GameServer server(port);
  return 0;
}
