#include "ScriptManager.h"
#include <sol/sol.hpp>
#include "Types.h"

ScriptManager::ScriptManager(EntityComponentSystem *e)
{
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
    lua.script("math.randomseed(0)");
    ecs = e;
}

bool ScriptManager::LoadScript( const string& name, const string& path )
{
    script_map[name] = lua.load_file( path );
    Script s = {name};
    EntityID id = ecs->emap.GetID(name);
    ecs->Create(id, s);

    //if the load_file was successful, return true.
    return true;
}

void ScriptManager::Update()
{
    ecs->ForEach<Script>( [&]( EntityID entity )
    {
        Script &s = ecs->Get<Script>(entity);
        script_map[s.name](entity);
    });
}
