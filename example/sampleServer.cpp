#include "../library/server.h"
#include <asio/connect.hpp>
#include <cstdint>

enum class MessageType : uint32_t
{
  ServerAccept,
  ServerDeny,
  ServerPing, 
  MessageAll,
  ServerMessage
};

class SampleServer final : public net_frame::server_interface<MessageType>
{
public:
  SampleServer(uint16_t nPort) : net_frame::server_interface<MessageType>(nPort)
  {
  }

protected:
  bool OnClientConnect(std::shared_ptr<net_frame::connection<MessageType>> client)
  override
  {
    return true;
  }

  void OnMessage(std::shared_ptr<net_frame::connection<MessageType>> client, net_frame::message<MessageType>& msg)
  override
  {
    
    switch(msg.header.id)
    {
      case MessageType::ServerPing:
      {
        std::chrono::system_clock::time_point timeThen;        

        // Bounce message back
        client->Send(msg);          
      }
      break;
      default:
      break;
    }
  }
};

int main()
{
  SampleServer server(60'000);
  server.Start();
  while(1)
  {
    server.Update(-1, true);
  }
}
