#include <cstdint>

const uint16_t PORT = 60'000;

enum class MessageType : uint32_t
{
  ServerAccept,
  Move,
  GameEnd,
};

struct Move{
  uint8_t x;
  uint8_t y;
};

enum Symbol{
  None,
  X,
  O
};

struct Board{
  char map[3][3]{0};

  bool check_valid_move(Move move){
    return move.x<=2 && move.y<=2 && map[move.y][move.x]==0;
  }
  void apply_move(const Move move, char symbol){
    map[move.y][move.x] = symbol;
  }
  char check_win(){
    // horizontal
    for(int i=0;i<3;++i)
      if(map[0][i]!=0 && map[0][i] == (map[0][i] & map[1][i] & map[2][i]))
        return map[0][i];

    // vertical
    for(int i=0;i<3;++i)
      if(map[i][0]!=0 && map[i][0] == (map[i][0] & map[i][1] & map[i][2]))
        return map[i][0];

    // diagnal
    if(map[0][0]!=0 && map[0][0] == (map[0][0] & map[1][1] & map[2][2]))
      return map[0][0];
    if(map[0][2]!=0 && map[0][2] == (map[0][2] & map[1][1] & map[2][0]))
      return map[0][2];

    // no win
    return 0;
  }
};

