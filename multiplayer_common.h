#pragma once

#include <cstdint>
#include <unordered_map>
#include <functional>
#include <climits>
#include <limits>

#include "include/raylib.h"
#include "include/RaylibOpOverloads.hpp"

#define GRID_SIZE 100.f
#define TILE_SIZE 300.f
#define GRID_COLOR DARKGREEN

#define WORLD_SIZE (int)(TILE_SIZE * 10)

enum class GameMsg : uint32_t
{
  Server_GetStatus,
  Server_GetPing,

  Client_Accepted,
  Client_AssignID,
  Client_RegisterWithServer,
  Client_UnregisterWithServer,

  Game_AddPlayer,
  Game_RemovePlayer,
  Game_UpdatePlayer,
  Game_UpdatePlayerLook
};

using idT = uint32_t;


typedef struct PlayerDescription 
{
  uint32_t uUniqueID = 0;
  
  idT lvl = 0;

  struct PlayerLookDesc
  {
    Color color = WHITE;
    Color left_hand_color = BLUE;
    Color right_hand_color = RED;
    static constexpr Vector2 left_hand_pos = {-10, 15};
    static constexpr Vector2 right_hand_pos = {10, 15};
    static constexpr float hand_width = 15;
    static constexpr float hand_length = 40;
  } look;

  struct PlayerPhysDesc
  {
    Vector2 pos = {0,0};
    Vector2 vel = {0,0};
    Vector2 handsDir = {0,1};
    static constexpr float acc = 1000.f;
    static constexpr float radius = 30.f;
  } phys;
  
  PlayerDescription() = default;
  PlayerDescription(Vector2 pos){
    phys.pos = pos;
  }

  friend bool operator==(const PlayerDescription& p1, const PlayerDescription& p2)
  {return p1.uUniqueID == p2.uUniqueID;}
} Player;


inline void UpdatePlayerCollisions(
  PlayerDescription& pdata, 
  std::unordered_map<idT, PlayerDescription>& players,
  std::function<void(idT)> affected = [](idT id){})
{
  auto player = pdata.phys;


  // // World border collisions
  // if(player.pos.x + player.radius >= WORLD_SIZE)
  // {
  //   player.pos.x = WORLD_SIZE - player.radius;
  //   player.vel.y = - abs(player.vel.y);
  // }
  // if(player.pos.x - player.radius <= 0)
  // {
  //   player.pos.x = player.radius;
  //   player.vel.y = abs(player.vel.y);
  // }
  // if(player.pos.y + player.radius >= WORLD_SIZE)
  // {
  //   player.pos.y = WORLD_SIZE - player.radius;
  //   player.vel.x = - abs(player.vel.x);
  // }
  // if(player.pos.y - player.radius <= 0)
  // {
  //   player.pos.y = player.radius;
  //   player.vel.x = abs(player.vel.x);
  // }
  
  
  for(auto& d : players)
  {
    auto& target = d.second.phys;
    if(d.second == pdata) continue;

    if(CheckCollisionCircles(player.pos, player.radius, target.pos, target.radius))
    {
      float dist = Vector2Distance(player.pos, target.pos)+std::numeric_limits<float>::epsilon();
      float overlap = 0.5f * (dist - player.radius - target.radius);

      Vector2 normal = (target.pos - player.pos)/dist; // tangent -> player
      // Vector2 tangent = {-normal.y,normal.x};  
      
      // // tangent response
      // float tan1 = Vector2DotProduct(player.vel, tangent);
      // float tan2 = Vector2DotProduct(target.vel, tangent);
      // // normal response
      // float norm1 = Vector2DotProduct(player.vel, normal);
      // float norm2 = Vector2DotProduct(target.vel, normal);


      
      // normal momentum is exchanged, because they are the same mass
      // player.vel = tangent * tan1 + normal * norm2;
      // target.vel = tangent * tan2 + normal * norm1;
      
      // apply offset collisions
      player.pos += normal * overlap;
      target.pos -= normal * overlap;

      // simplified version
      Vector2 k = player.vel - target.vel;
      float p = normal.x * k.x + normal.y * k.y;
      player.vel -= normal * p;
      target.vel += normal * p;
      
      affected(d.second.uUniqueID);
    }
  }
}
