#include "../../library/server.h"
#include <asio/connect.hpp>
#include <cstdint>
#include "common.h"
#include <algorithm>
#include <map>
#include <random>



class SampleServer final : public sonicpp::ServerInterface<MessageType>
{
  const std::vector<char> symbols{'o', 'x'};
  std::vector<char> symbols_left{symbols};
  std::map<uint32_t, char> players{};
  bool started = false;
  char round = 'o';
  Board board{};
public:
  SampleServer(uint16_t nPort) : sonicpp::ServerInterface<MessageType>(nPort)
  {
  }

  
protected:
  bool OnClientConnect(std::shared_ptr<sonicpp::Connection<MessageType>> client)
  override
  {
    return !symbols_left.empty();
  }
  
  void OnClientDisconnect(std::shared_ptr<sonicpp::Connection<MessageType>> client)
  override
  {
    symbols_left.push_back(players[client->GetID()]);
    players.erase(client->GetID());

    std::cout << "[SERVER] Player ["<< client->GetID() << "] quit, " << players.size() << "/2 players left" << std::endl;
    started = false;
    new (&board) Board();
  }
  
  void OnClientValidated(std::shared_ptr<sonicpp::Connection<MessageType>> client)
  override
  {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(symbols_left.begin(), symbols_left.end(), g);

    players.insert({client.get()->GetID(), symbols_left.back()});
    symbols_left.pop_back();

    std::cout << "[SERVER] " << players.size() << "/2 players in" << std::endl;
    
    // if both clients have connected
    if(symbols_left.empty()){
      std::cout << "[SERVER] GAME STARTED!!!" << std::endl;
      // notify all clients about thier symbols
      for(auto& client : m_deqConnections){
        Message msg{MessageType::ServerAccept};
        msg << players[client->GetID()];
        MessageClient(client, msg);
      }
      started = true;
    }
  }

  void OnMessage(std::shared_ptr<sonicpp::Connection<MessageType>> client, sonicpp::Message<MessageType>& msg)
  override
  {    
    switch(msg.header.id)
    {
      case MessageType::Move:
      {
        if(!started)
          break;
        
        Move move;
        msg >> move;
        if(!board.check_valid_move(move)){
          delete this;
          std::cerr << "Invalid move reveived!"<< std::endl;
          exit(1);
        }
        std::cout << "[SERVER] Move received (" << static_cast<int>(move.y) << ',' << static_cast<int>(move.x) << ')' << std::endl; 

        char symbol = players[client.get()->GetID()];
        board.apply_move(move,symbol);

        // next round 
        round = (round=='o')?'x':'o';

        char win = board.check_win();
        char next_player = (win==0)?round:0;

        Message msg{MessageType::Move};
        msg << move << symbol << next_player;
        MessageAllClients(msg);

        if(win){
          Message msg{MessageType::GameEnd};
          msg << win;   
          MessageAllClients(msg);
        }
      }
      break;
      default:
        std::cout << "[SERVER] Unexpected message received!" << std::endl;
      break;
    }
  }
};


int main()
{
  SampleServer server(PORT);
  server.Start();
  while(1)
  {
    server.Update(-1, true);
  }
}
