#pragma once
#include "glm/glm.hpp"
#include <string>
#include <unordered_map>

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;
typedef std::string string;

struct Uniforms {
    mat4 projection;
};

struct InstanceData {
    vec3 translation;
    vec2 scale;
    // rotation?
};

struct LifeSprite{ char dummy; };
struct Position : public vec2{};
struct Velocity : public vec2{};
struct Gravity {float a;};
struct Health {int hp;};
struct HitRate {int rate;};
struct Sprite { string image; vec2 size; };
struct Script { string name; };
struct Collider
{ 
    vec2 size;
    bool trigger;
};