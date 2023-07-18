#include <climits>
#include <cstdint>
#include <thread>
#include <cmath>
#include <unordered_map>

// RAYLIB
#include "include/raylib.h"
#include "include/RaylibOpOverloads.hpp"
#define RAYGUI_IMPLEMENTATION
#include "include/raygui.h"
#include "style_cyber.h"

#include "library/client.h"
#include "library/message.h"
#include "multiplayer_common.h"


using namespace std::chrono_literals;

class Game : public net_frame::client_intefrace<GameMsg>
{
  
  const Vector2 screenSize = {1200,800};
public:
  Game(std::string& ip, uint16_t port)
  {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenSize.x, screenSize.y, "Game");
    SetTargetFPS(60);
    
    mainCamera = {0};
    mainCamera.offset = screenSize/2.f;
    mainCamera.target = {0,0};
    mainCamera.rotation = 0.f;
    mainCamera.zoom = 1.0f;

    // Gui related
    GuiLoadStyleCyber();

    // Game logic related
    while(!UserCreate(ip, port))
      std::this_thread::sleep_for(1s);
    
    while (!WindowShouldClose())
    {
        Update(GetFrameTime());
        BeginDrawing();
          DrawScreen();
          DrawFPS(10, 10);
        EndDrawing();
    }
  }

private:
  Camera2D mainCamera;
  
  std::unordered_map<idT, PlayerDescription> players;
  idT thisPlayerID = 0;
  PlayerDescription descPlayer;
  bool bWaitingForConnection = true;
  
  bool lookChangeMenu = false;
  bool mainMenu = false;
  PlayerDescription::PlayerLookDesc tempLook;

  Vector2 inline ClosestPointLine(const Vector2 l1, const Vector2 l2, const Vector2 p)
  {
    if(l1==l2) return l1;
    
    float A1 = l2.y - l1.y; 
    float B1 = l1.x - l2.x; 
    float C1 = (l2.y - l1.y)*l1.x + (l1.x - l2.x)*l1.y; 
    float C2 = -B1*p.x + A1*p.y; 
    float det = A1*A1 - -B1*B1; 
    float cx = 0; 
    float cy = 0; 
    if(det != 0){ 
    cx = (float)((A1*C1 - B1*C2)/det); 
    cy = (float)((A1*C2 - -B1*C1)/det); 
    }else{ 
    cx = p.x; 
    cy = p.y; 
    } 
    return Vector2{cx, cy};     
  }
  
  bool inline PlayerCollide(PlayerDescription::PlayerPhysDesc& p1, PlayerDescription::PlayerPhysDesc& p2, float dt)
  {
    if(&p1==&p2) return false;

    
    Vector2 d = ClosestPointLine(p1.pos, p1.pos+p1.vel*dt, p2.pos);
    DrawCircleV(d, 10, BLUE);
    float closestDistSq = Vector2DistanceSqr(d, p2.pos);
    if(closestDistSq <= pow(p1.radius + p2.radius,2))
    {
      // a collision has occured
      return true;
      // p2.vel = -p;
    }
    return false;
  }
  
  void Update(float dt)
  {
    // Check for incoming messages
    if(IsConnected())
    {
      while(!Incoming().is_empty())
      {
        auto msg = Incoming().pop_front().msg;

        switch(msg.header.id)
        {
          // We have been accepted as a client
          // we can't yet interact though
          case GameMsg::Client_Accepted:
          {
              std::cout << "Server Accepted you!" << std::endl;
              message msgSend;
              msgSend.header.id = GameMsg::Client_RegisterWithServer;

              descPlayer = PlayerDescription({GetRenderWidth()/2.f, GetRenderHeight()/2.f});
              msgSend << descPlayer;
            
              Send(msgSend);
          }
          break;
          // Server assigned an ClientID for us
          case GameMsg::Client_AssignID:
          {
              msg >> thisPlayerID;
              std::cout << "Server provided your id: " << thisPlayerID << std::endl;
          }
          break;
          // Server added a new player, that could be us
          case GameMsg::Game_AddPlayer:
          {
            PlayerDescription desc;
            msg >> desc;
            players[desc.uUniqueID] = desc; 

            std::cout << "Server added player: " << desc.uUniqueID << std::endl;
            // Thats us, we have been finally added to game by the server
            if(desc.uUniqueID == thisPlayerID)
            {
                // now we can play
                bWaitingForConnection = false;
            }
          }
          break;
          // Server removed a player
          case GameMsg::Game_RemovePlayer:
          {
            idT removeId;
            msg >> removeId;
            players.erase(removeId);              
          }
          break;
          // Update some player object, could be us
          case GameMsg::Game_UpdatePlayer:
          {
            idT id;
            PlayerDescription::PlayerPhysDesc physDesc;
            msg >> physDesc >> id;
            
            // desc.vel = players[desc.uUniqueID].vel;
            players[id].phys = physDesc;
              
            UpdatePlayerCollisions(players[id], players);
          }
          break;
          case GameMsg::Game_UpdatePlayerLook:
          {
            // PlayerDescription::PlayerLookDesc newLook;
            idT id;
            PlayerDescription::PlayerLookDesc newLook;
            msg >> newLook >> id;
            players[id].look = newLook;              
          }
          break;
          // break;
          // case GameMsg::Client_Accepted:
          // {
              
          // }
          // break;
          default:
          break;
        }
      }
    }

    const Vector2 screenCenter = {GetRenderWidth()/2.f, GetRenderHeight()/2.f};
    
    if(bWaitingForConnection)
       return;
    
    auto& player = players[thisPlayerID].phys;

    Vector2 mouseDir = Vector2Normalize(GetMousePosition() - screenCenter);
    player.handsDir = mouseDir;
    
    Vector2 delta_v = {0,0};
    
    if(IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
      delta_v += Vector2{0,-1};
    if(IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
      delta_v += Vector2{0,1};
    if(IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
      delta_v += Vector2{-1,0};
    if(IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
      delta_v += Vector2{1,0};
    
    if(IsKeyPressed(KEY_E))
    {
      // if character was changed, upload to server
      if(!lookChangeMenu)
      {
        // just opened the menu
        tempLook = players[thisPlayerID].look;
      }
      lookChangeMenu = !lookChangeMenu;
    }
    delta_v = Vector2Normalize(delta_v) * player.acc * dt;
      

    const float deceleration = 100 * dt;
    if(Vector2DistanceSqr({0,0}, player.vel+delta_v) > deceleration)
    { 
      // Check Collisions
      // for(auto& otherPlayer : players)
        // PlayerCollide(player, otherPlayer.second);          

      // add deceleration
      delta_v -= Vector2Normalize(player.vel) * deceleration; 
      player.vel += delta_v; //* 0.5f;
      player.pos += player.vel * dt;
      // player.vel += delta_v * 0.5f;

    }
    else 
      // stop
      player.vel = delta_v = {0,0};    
    
    // Send myPlayer data to server
    message msg{};
    msg.header.id = GameMsg::Game_UpdatePlayer;
    msg << thisPlayerID << players[thisPlayerID].phys;
    Send(msg);

    // Collision on client side
    for(auto& d1 : players)
    {
      auto& ball = d1.second;
      UpdatePlayerCollisions(ball, players);
    }

    
    
    return;
  }

  
  void draw_grid_background()
  {
      const int tile_size = 50;
      for (int i = -GRID_SIZE; i < GRID_SIZE; i ++)
      {
          DrawLineEx((Vector2){GRID_SIZE * TILE_SIZE, TILE_SIZE*i}, (Vector2){-GRID_SIZE * TILE_SIZE, TILE_SIZE*i}, 1, GRID_COLOR);

          DrawLineEx((Vector2){TILE_SIZE*i, GRID_SIZE * TILE_SIZE}, (Vector2){TILE_SIZE*i, -GRID_SIZE * TILE_SIZE}, 1, GRID_COLOR);
      }
  }


  char mainMenu_ipAddr[sizeof("255.255.255.255")] = {0};
  char mainMenu_port[sizeof("9999")] = {0};  
  void DrawScreen()
  {
    const int FONT_SIZE = 20;
    const Vector2 screenCenter = {GetRenderWidth()/2.f, GetRenderHeight()/2.f};

    if(mainMenu)
    {

      
      
      return;
    }
    
    // Blue screen
    if(bWaitingForConnection)
    {
      const int FONT_SIZE = 60;
      ClearBackground(DARKBLUE);
      
      const char* label = "Waiting for connection to server";
      const Vector2 labelSize = MeasureTextEx(GetFontDefault(), label, FONT_SIZE, 2);
      DrawTextEx(GetFontDefault(), label, {(GetRenderWidth()-labelSize.x)/2.f, (GetRenderHeight()-labelSize.y)/2.f}, FONT_SIZE, 2, WHITE);
      return;
    }

    if(lookChangeMenu)
    {
      ClearBackground(GRAY);
      
      GuiColorPicker({100,100,100,100}, "Body color", &tempLook.color);
      GuiColorPicker({100,220,100,100}, "Left hand color", &tempLook.left_hand_color);
      GuiColorPicker({100,340,100,100}, "Right hand color", &tempLook.right_hand_color);
      
      Vector2 mouseDir = Vector2Normalize(GetMousePosition() - screenCenter);

      const int SCALE = GetRenderWidth() * 5 / screenSize.x;
      const float radius = PlayerDescription::PlayerPhysDesc::radius * SCALE;
      
      DrawCircleV(screenCenter, radius, tempLook.color);
          
      // DrawPlayer Hands
      DrawLineEx(screenCenter+tempLook.left_hand_pos * SCALE, screenCenter+(tempLook.left_hand_pos+mouseDir*tempLook.hand_length) * SCALE, tempLook.hand_width * SCALE, tempLook.left_hand_color);
      DrawLineEx(screenCenter+tempLook.right_hand_pos * SCALE, screenCenter+(tempLook.right_hand_pos+mouseDir*tempLook.hand_length) * SCALE, tempLook.hand_width * SCALE, tempLook.right_hand_color);
      
      const char* label = TextFormat("ID: %d",thisPlayerID);
      const Vector2 labelSize = MeasureTextEx(GetFontDefault(), label, FONT_SIZE * SCALE, 2);
      DrawTextEx(GetFontDefault(), label, screenCenter + Vector2{-labelSize.x/2.f,- radius - labelSize.y}, FONT_SIZE * SCALE, 2, BLACK);
      
      
      if(GuiButton({screenCenter.x, GetScreenHeight() - 40.f, 100,50}, "Save"))
      {
        // save tempLook to accual player
        players[thisPlayerID].look = tempLook;
        // Send new look to server
        message changeLook;
        changeLook.header.id = GameMsg::Game_UpdatePlayerLook;
        changeLook << thisPlayerID << players[thisPlayerID].look;
        Send(changeLook);

        lookChangeMenu = false;
      }
      
      return;
    }
    
    ClearBackground(GRAY);

      
    // Draw players relative to camera
    mainCamera.target = players[thisPlayerID].phys.pos;
    mainCamera.offset = screenCenter;
    
    BeginMode2D(mainCamera);

    // grid with be moving 
    draw_grid_background();

    // DrawRectangle(-WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE, WORLD_SIZE*3, BLACK);
    // DrawRectangle(WORLD_SIZE, -WORLD_SIZE, WORLD_SIZE, WORLD_SIZE*3, BLACK);
    
    // DrawRectangle(0, -WORLD_SIZE, WORLD_SIZE, WORLD_SIZE, BLACK);
    // DrawRectangle(0, WORLD_SIZE, WORLD_SIZE, WORLD_SIZE, BLACK);
    
    for(auto& object : players)
    {
      idT id = object.first;
      auto& p = object.second;


      // DrawPlayer Bodu
      DrawCircleV(p.phys.pos, p.phys.radius, p.look.color);
      
      // DrawVelocityIndicator
      DrawLineV(p.phys.pos, p.phys.pos+Vector2Normalize(p.phys.vel)*p.phys.radius, RED);
      
      // DrawPlayer Hands
      DrawLineEx(p.phys.pos+p.look.left_hand_pos, p.phys.pos+p.look.left_hand_pos+p.phys.handsDir*p.look.hand_length, p.look.hand_width, p.look.left_hand_color);
      DrawLineEx(p.phys.pos+p.look.right_hand_pos, p.phys.pos+p.look.right_hand_pos+p.phys.handsDir*p.look.hand_length, p.look.hand_width, p.look.right_hand_color);
      
      const char* label = TextFormat("ID: %d",p.uUniqueID);
      const Vector2 labelSize = MeasureTextEx(GetFontDefault(), label, FONT_SIZE, 2);
      DrawTextEx(GetFontDefault(), label, p.phys.pos + Vector2{-labelSize.x/2.f,- p.phys.radius - labelSize.y}, FONT_SIZE, 2, BLACK);
      
      // If player is on the edge, then draw it once again on the other edge
      // if()      
    }
    EndMode2D();
    
    DrawText(TextFormat("[%f, %f]",players[thisPlayerID].phys.pos.x,players[thisPlayerID].phys.pos.y), 30,30,20,RED);
  }
  
public:
  bool UserCreate(std::string& ip, uint16_t port)
  {
    
    // players.insert(std::make_pair(0, Player(Vector2{GetScreenWidth()/2.f,GetScreenHeight()/2.f})));
    if(Connect(ip, port))
    {
      return true;
    }

    std::cerr << "Connection to server failed!" << std::endl;
    return false;
  }
};

int main(int argc, char* argv[]) // first argument is the program itself
{
  
  std::string ip_addr = "127.0.0.1";
  uint16_t port = 60'000;
  if(argc == 1)
  {
    std::cout << "[INFO] using default server ip: " << ip_addr << std::endl;
    std::cout << "[INFO] using default port: " << port << std::endl;
  }
  if(argc == 2)
  {
    std::cout << "[INFO] using default port: " << port << std::endl;
    ip_addr = argv[1]; 
  }
  if(argc == 3)
  {
    ip_addr = argv[1];
    port = std::stoi(argv[2]); 
  }
  
  Game game(ip_addr, port);
}
