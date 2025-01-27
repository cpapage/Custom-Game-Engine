#pragma once
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "Types.h"
#include "EntityComponentSystem.h"
#include <unordered_map>

class ScriptManager
{
public:
    sol::state lua;
    std::unordered_map<string, sol::load_result> script_map;
    EntityComponentSystem *ecs;

    ScriptManager(EntityComponentSystem *e);
    bool LoadScript( const string& name, const string& path );
    void Update();
};