#include <iostream>
#include <unistd.h>
#include "../../library/client.h"
#include "common.h"


class SampleClient final : public sonicpp::ClientIntefrace<MessageType>
{
  char my_symbol=0;
  Board board{};
  bool my_round=false;
public:
  SampleClient(){
    if(!Connect("127.0.0.1", PORT))
      exit(1);

  
    bool loop = true;
    while(loop)
    {
      if(IsConnected())
      {
        auto msg = AwaitNextMessage();
          switch(msg.GetType())
          {
            case MessageType::ServerAccept:
            {
              msg >> my_symbol;
              if(my_symbol=='o')
                my_round=true;
            } 
            break;
            case MessageType::Move:
            {
              Move move;
              char symbol;
              char round;
              msg >> round >> symbol >> move;
              board.apply_move(move, symbol);

              render(); // render immidietly

              my_round = (round==my_symbol);
            }
            break;
            case MessageType::GameEnd:{
              char winner;
              msg >> winner;
              if(winner==my_symbol)
                std::cout << "YOU WON!!!" << std::endl;
              else
                std::cout << "YOU LOST!!!" << std::endl;

              Disconnect();
              exit(0);
            }
            break;
          }
      }
      else
      {
        std::cout << "Server Down!" << std::endl;
        exit(1);
      }

      if(my_symbol==0 || !my_round)
        continue;

      Move m{};
      do{
        render();
        m = get_move();
      }while(!board.check_valid_move(m));
      my_round = false;

      Message msg{MessageType::Move};
      msg << m;
      Send(msg);
    }  
  }

  
  void render(){
    std::cout << "\033c";
    for(int i=0;i<3;++i){
      for(int j=0;j<3;++j)
        std::cout << ((board.map[i][j]==0)?' ':board.map[i][j]);
      std::cout << '\n';
      
    }
    std::cout << std::endl;
  }
  
  Move get_move()
  {    
      std::cout << "Enter move: " << std::endl;
      int _y,_x;
      std::cin >> _y >> _x;
      return Move{.x=static_cast<uint8_t>(_x), .y=static_cast<uint8_t>(_y)};
  }
};


int main()
{
  SampleClient c{};
  return 0;  
}
