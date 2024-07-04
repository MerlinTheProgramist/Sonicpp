#pragma once

#include <cstdint>
#include <functional>
#include <limits>

#include <raylib-cpp.hpp>
#include <raylib.h>

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
    raylib::Color color = WHITE;
    raylib::Color left_hand_color = BLUE;
    raylib::Color right_hand_color = RED;
    static const raylib::Vector2 left_hand_pos;
    static const raylib::Vector2 right_hand_pos;
    static constexpr float hand_width = 15;
    static constexpr float hand_length = 40;
  } look;

  struct PlayerPhysDesc
  {
    raylib::Vector2 pos = {0,0};
    raylib::Vector2 vel = {0,0};
    raylib::Vector2 handsDir = {0,1};
    static constexpr float acc = 1000.f;
    static constexpr float radius = 30.f;
    
    bool Update(float deltaTime, raylib::Vector2 delta_v = {0,0})
    {
      const float deceleration = 100 * deltaTime;
      if(Vector2DistanceSqr({0,0}, vel+delta_v) > deceleration)
      { 

        // add deceleration
        delta_v -= vel.Normalize() * deceleration; 
        vel += delta_v; //* 0.5f;
        pos += vel * deltaTime;
        // vel += delta_v * 0.5f;
        return true;
      }
      // stop
      vel = raylib::Vector2{0,0};    
      return false;
    }
  } phys;
  
  PlayerDescription() = default;
  PlayerDescription(Vector2 pos){
    phys.pos = pos;
  }

  friend bool operator==(const PlayerDescription& p1, const PlayerDescription& p2)
  {return p1.uUniqueID == p2.uUniqueID;}
  
  
} Player;


inline bool UpdatePlayerCollisions(
  PlayerDescription::PlayerPhysDesc& playerA,
  PlayerDescription::PlayerPhysDesc& playerB, 
  std::function<void(idT)> affected = [](idT id){})
{
  // Check collision
  if(!CheckCollisionCircles(playerA.pos, playerA.radius, playerB.pos, playerB.radius))
    return false;

  // Apply appropriet physical reaction
  float dist = Vector2Distance(playerA.pos, playerB.pos)+std::numeric_limits<float>::epsilon();
  float overlap = 0.5f * (dist - playerA.radius - playerB.radius);

  raylib::Vector2 normal = (playerB.pos - playerA.pos)/dist; // tangent -> player
  // apply offset collisions
  playerA.pos += normal * overlap;
  playerB.pos -= normal * overlap;
  
  // Long version 
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

  // simplified version
  raylib::Vector2 k = playerA.vel - playerB.vel;
  float p = normal.x * k.x + normal.y * k.y;
  playerA.vel -= normal * p;
  playerB.vel += normal * p;

  return true;
}

inline void UpdatePlayerPeramiterCollisions(
  PlayerDescription::PlayerPhysDesc& player)
{
  // // World border collisions
  if(player.pos.x + player.radius >= WORLD_SIZE)
  {
    player.pos.x = WORLD_SIZE - player.radius;
    player.vel.y = - abs(player.vel.y);
  }
  if(player.pos.x - player.radius <= 0)
  {
    player.pos.x = player.radius;
    player.vel.y = abs(player.vel.y);
  }
  if(player.pos.y + player.radius >= WORLD_SIZE)
  {
    player.pos.y = WORLD_SIZE - player.radius;
    player.vel.x = - abs(player.vel.x);
  }
  if(player.pos.y - player.radius <= 0)
  {
    player.pos.y = player.radius;
    player.vel.x = abs(player.vel.x);
  }
}
const raylib::Vector2 PlayerDescription::PlayerLookDesc::left_hand_pos{-10, 15};
const raylib::Vector2 PlayerDescription::PlayerLookDesc::right_hand_pos{10, 15};
