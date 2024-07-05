#include <chrono>

#include <cstdint>
#include <iostream>
#include <thread>
#include <unistd.h>
#include "../library/client.h"

enum class MessageType : uint32_t
{
  ServerAccept,
  ServerDeny,
  ServerPing, 
  MessageAll,
  ServerMessage
};

class SampleClient final : public sonicpp::ClientIntefrace<MessageType>
{
public:
  SampleClient(){
    if(!Connect("127.0.0.1", 60'000))
      exit(1);

    std::thread pingTh([this]()
    {
      for(;;)
      {
        std::this_thread::sleep_for(1s);
        PingServer();
      }   
    });
  
    bool loop = true;
    while(loop)
    {
      if(IsConnected())
      {
        while(auto msg = NextMessage())
        {

          switch(msg->header.id)
          {
            case MessageType::ServerPing:
            {
              std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
              std::chrono::system_clock::time_point timeThen;
              *msg >> timeThen;

              std::cout << "Timepoint received: " << std::chrono::system_clock::to_time_t(timeThen) << std::endl;
              std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "ms" << std::endl;
            }
            break;
            default:
            break;
          }
        }
      }
      else
      {
        std::cout << "Server Down!" << std::endl;
        loop = false;
      }
    }
    if(pingTh.joinable()) pingTh.join();
  
  }
public:
  void PingServer()
  {
    sonicpp::Message<MessageType> msg;
    msg.header.id = MessageType::ServerPing;

    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
    msg << timeNow;
    Send(msg);   
    msg >> timeNow;
    std::cout << "Timepoint sent: " << std::chrono::system_clock::to_time_t(timeNow) << std::endl; 
  }
};

static std::string getKey()
{    
    std::string answer;
    std::cin >> answer;
    return answer;
}


using namespace std::chrono_literals;
int main()
{
  SampleClient c{};
  
  return 0;
  
}
