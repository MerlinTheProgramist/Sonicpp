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

class SampleServer final : public sonicpp::ServerInterface<MessageType>
{
public:
  SampleServer(uint16_t nPort) : sonicpp::ServerInterface<MessageType>(nPort)
  {
  }

protected:
  bool OnClientConnect(std::shared_ptr<sonicpp::Connection<MessageType>> client)
  override
  {
    return true;
  }

  void OnMessage(std::shared_ptr<sonicpp::Connection<MessageType>> client, sonicpp::Message<MessageType>& msg)
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
