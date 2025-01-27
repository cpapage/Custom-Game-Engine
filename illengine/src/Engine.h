#pragma once
#include "GraphicsManager.h"
#include "InputManager.h"
#include "EntityComponentSystem.h"
#include "GLFW/glfw3.h"
#include "ScriptManager.h"
#include <chrono>
#include <thread>
#include <functional>
#include <vector>
#include <sol/sol.hpp>
typedef std::function<void()> UpdateCallback;

class Engine
{
public:
    GraphicsManager* graphics;
    InputManager* input;
    ScriptManager* scripts;
    EntityComponentSystem *ecs;

    Engine(int width = int(0), int height = int(0), const char* name = NULL, bool fullscreen = false) 
    {
        ecs = new EntityComponentSystem();
        graphics = new GraphicsManager(width, height, name, fullscreen, ecs);
        input = new InputManager(graphics);
        scripts = new ScriptManager(ecs);
        scripts->lua.set_function( "KeyIsDown", [&]( const int keycode ) { return input->KeyIsPressed( keycode ); } );
        scripts->lua.new_enum( "KEYBOARD",
            "SPACE", GLFW_KEY_SPACE, "W", GLFW_KEY_W, "S", GLFW_KEY_S, "A", GLFW_KEY_A, "D", GLFW_KEY_D, "ENTER", GLFW_KEY_ENTER
        );
        scripts->lua.set_function( "ShouldQuit", [&](void){graphics->SetShouldQuit(true);});
        scripts->lua.set_function( "GetSprite", [&]( EntityID e ) -> Sprite& { return ecs->Get<Sprite>(e); });
        scripts->lua.set_function( "GetPosition", [&]( EntityID e ) -> vec2& { return ecs->Get<Position>(e); });
        scripts->lua.set_function( "GetVelocity", [&]( EntityID e ) -> vec2& { return ecs->Get<Velocity>(e); });
        scripts->lua.set_function( "GetGravity", [&]( EntityID e ) -> float& { return ecs->Get<Gravity>(e).a; });
        scripts->lua.set_function( "GetCollider", [&]( EntityID e ) -> vec2& { return ecs->Get<Collider>(e).size; });
        scripts->lua.set_function( "IsTrigger", [&]( EntityID e ) -> bool& { return ecs->Get<Collider>(e).trigger; });
        scripts->lua.set_function( "GetHealth", [&]( EntityID e ) -> int& { return ecs->Get<Health>(e).hp; });
        scripts->lua.set_function( "GetHitRate", [&]( EntityID e ) -> int& { return ecs->Get<HitRate>(e).rate; });
        scripts->lua.set_function( "SetHitRate", [&]( EntityID e, int new_value ) { ecs->Get<HitRate>(e).rate = new_value; });
        scripts->lua.set_function( "SetHealth", [&]( EntityID e, int new_value ) { ecs->Get<Health>(e).hp = new_value; });
        scripts->lua.set_function( "SetGravity", [&]( EntityID e, float new_value ) { ecs->Get<Gravity>(e).a = new_value; });
        scripts->lua.set_function( "Destroy" , [&]( EntityID e ){ecs->Destroy(e);});
        scripts->lua.set_function( "GameOver" , [&](void){
            ecs->Destroy(ecs->emap.GetID("Triceratops"));
            graphics->LoadOneImage("Game Over", "assets/Game Over screen.png");
            EntityID e = ecs->emap.GetID("Game Over");
            Sprite &s = ecs->Get<Sprite>(e);
            s.size.x = 1980;
            s.size.y = 1080;
            Position p;
            p.x = 0;
            p.y = 0;
            ecs->Create(e, p);
        });
        scripts->lua.set_function( "Victory" , [&](void){
            ecs->Destroy(ecs->emap.GetID("Triceratops"));
            graphics->LoadOneImage("Victory", "assets/Victory.png");
            EntityID e = ecs->emap.GetID("Victory");
            Sprite &s = ecs->Get<Sprite>(e);
            s.size.x = 1980;
            s.size.y = 1080;
            Position p;
            p.x = 0;
            p.y = 0;
            ecs->Create(e, p);
        });
        scripts->lua.set_function( "GetLives" , [&](void)
        {
            std::vector<EntityID> array;
            ecs->ForEach<LifeSprite>( [&] (EntityID entity)
            {
                array.push_back(entity);
            });
            return array;
        });
        scripts->lua.set_function( "GetPlayer" , [&](void) -> EntityID {return ecs->emap.GetID("Triceratops"); });
        scripts->lua.set_function( "Collides" , [&]( EntityID e1, EntityID e2 ) -> bool {
            vec2 c1 = ecs->Get<Collider>(e1).size;
            vec2 c2 = ecs->Get<Collider>(e2).size;
            Position p1 = ecs->Get<Position>(e1);
            Position p2 = ecs->Get<Position>(e2);
            return (p1.y - c1.y/2 <= p2.y + c2.y/2 
                && p1.y + c1.y/2 >= p2.y - c2.y/2 
                && p1.x + c1.x/2 >= p2.x - c2.x/2 
                && p1.x - c1.x/2 <= p2.x + c2.x/2);
        });
        scripts->lua.set_function( "GetColliders", [&](void){
            std::vector<EntityID> array;
            ecs->ForEach<Collider>( [&]( EntityID entity)
            {
                array.push_back(entity);
            });
            return array;
        });
        scripts->lua.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            // optional and fancy: operator overloading. see: https://github.com/ThePhD/sol2/issues/547
            sol::meta_function::addition, sol::overload( [](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1+v2; } ),
            sol::meta_function::subtraction, sol::overload( [](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1-v2; } ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1*v2; },
                [](const glm::vec3& v1, float f) -> glm::vec3 { return v1*f; },
                [](float f, const glm::vec3& v1) -> glm::vec3 { return f*v1; }
            )
        );

        scripts->lua.new_usertype<glm::vec2>("vec2",
        sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            // optional and fancy: operator overloading. see: https://github.com/ThePhD/sol2/issues/547
            sol::meta_function::addition, sol::overload( [](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1+v2; } ),
            sol::meta_function::subtraction, sol::overload( [](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1-v2; } ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1*v2; },
                [](const glm::vec2& v1, float f) -> glm::vec2 { return v1*f; },
                [](float f, const glm::vec2& v1) -> glm::vec2 { return f*v1; }
            )
        );
    }

    void Startup()
    {
        graphics->Startup();
        input->Startup();
        scripts->LoadScript("Triceratops", "assets/Player.lua");
        scripts->LoadScript("Obstacle1", "assets/obstacle.lua");
        scripts->LoadScript("Obstacle2", "assets/obstacle.lua");
        scripts->LoadScript("Obstacle3", "assets/obstacle.lua");
        scripts->LoadScript("Goal", "assets/Win Condition.lua");
        graphics->LoadOneImage("Triceratops", "assets/Triceratops 2.jpg");
        graphics->LoadOneImage("Platform", "assets/Platform.png"); graphics->LoadOneImage("Platform2", "assets/Platform.png");
        graphics->LoadOneImage("Platform3", "assets/Platform.png"); graphics->LoadOneImage("Platform4", "assets/Platform.png");
        graphics->LoadOneImage("Platform5", "assets/Platform.png"); graphics->LoadOneImage("Platform6", "assets/Platform.png");
        graphics->LoadOneImage("Platform7", "assets/Platform.png"); graphics->LoadOneImage("Platform8", "assets/Platform.png");
        graphics->LoadOneImage("Platform9", "assets/Platform.png"); graphics->LoadOneImage("Platform10", "assets/Platform.png");
        graphics->LoadOneImage("Platform11", "assets/Platform.png"); graphics->LoadOneImage("Platform12", "assets/Platform.png");
        graphics->LoadOneImage("Platform13", "assets/Platform.png"); graphics->LoadOneImage("Platform14", "assets/Platform.png");
        graphics->LoadOneImage("Platform15", "assets/Platform.png"); graphics->LoadOneImage("Platform16", "assets/Platform.png");
        graphics->LoadOneImage("Platform17", "assets/Platform.png"); graphics->LoadOneImage("Platform18", "assets/Platform.png");
        graphics->LoadOneImage("Platform19", "assets/Platform.png"); graphics->LoadOneImage("Platform20", "assets/Platform.png");
        graphics->LoadOneImage("Platform21", "assets/Platform.png"); graphics->LoadOneImage("Platform22", "assets/Platform.png");
        graphics->LoadOneImage("Platform23", "assets/Platform.png"); graphics->LoadOneImage("Platform24", "assets/Platform.png");
        graphics->LoadOneImage("Platform25", "assets/Platform.png"); graphics->LoadOneImage("Platform26", "assets/Platform.png");
        graphics->LoadOneImage("Platform27", "assets/Platform.png"); graphics->LoadOneImage("Platform28", "assets/Platform.png");
        graphics->LoadOneImage("Platform29", "assets/Platform.png");

        graphics->LoadOneImage("Obstacle1", "assets/Obstacle.jpg");
        graphics->LoadOneImage("Obstacle2", "assets/Obstacle.jpg");
        graphics->LoadOneImage("Obstacle3", "assets/Obstacle.jpg");
        graphics->LoadOneImage("Start", "assets/Start.png");
        graphics->LoadOneImage("Goal", "assets/Goal.png");
        graphics->LoadOneImage("Life1", "assets/Life.png");
        graphics->LoadOneImage("Life2", "assets/Life.png");
        graphics->LoadOneImage("Life3", "assets/Life.png");
        graphics->LoadOneImage("Sign", "assets/Sign.png");

        Position p;
        Velocity v;
        Collider c;
        Gravity g = {0};
        c.size.x = 20;
        c.size.y = 20;
        c.trigger = false;
        p.x = -160;
        p.y = 0;
        v.x = 0;
        v.y = 0;

        // Map platforms. Floor 1 p.y = -20, Floor 2 p.y = -80
        p.y = -20;
        c.size.x = 20;
        ecs->Create(ecs->emap.GetID("Platform"), p);
        ecs->Create(ecs->emap.GetID("Platform"), c);
        p.x = -140;
        ecs->Create(ecs->emap.GetID("Platform2"), p);
        ecs->Create(ecs->emap.GetID("Platform2"), c);
        p.x = -120;
        ecs->Create(ecs->emap.GetID("Platform3"), p);
        ecs->Create(ecs->emap.GetID("Platform3"), c);
        p.x = -100;
        ecs->Create(ecs->emap.GetID("Platform4"), p);
        ecs->Create(ecs->emap.GetID("Platform4"), c);
        p.x = -80;
        ecs->Create(ecs->emap.GetID("Platform5"), p);
        ecs->Create(ecs->emap.GetID("Platform5"), c);
        p.x = -60;
        ecs->Create(ecs->emap.GetID("Platform6"), p);
        ecs->Create(ecs->emap.GetID("Platform6"), c);
        p.x = -40;
        ecs->Create(ecs->emap.GetID("Platform7"), p);
        ecs->Create(ecs->emap.GetID("Platform7"), c);
        p.x = -20;
        ecs->Create(ecs->emap.GetID("Platform8"), p);
        ecs->Create(ecs->emap.GetID("Platform8"), c);
        p.x = 0;
        ecs->Create(ecs->emap.GetID("Platform9"), p);
        ecs->Create(ecs->emap.GetID("Platform9"), c);
        p.x = 20;
        ecs->Create(ecs->emap.GetID("Platform10"), p);
        ecs->Create(ecs->emap.GetID("Platform10"), c);
        p.x = 40;
        ecs->Create(ecs->emap.GetID("Platform11"), p);
        ecs->Create(ecs->emap.GetID("Platform11"), c);
        p.x = 60;
        ecs->Create(ecs->emap.GetID("Platform12"), p);
        ecs->Create(ecs->emap.GetID("Platform12"), c);
        p.x = 80;
        ecs->Create(ecs->emap.GetID("Platform13"), p);
        ecs->Create(ecs->emap.GetID("Platform13"), c);
        p.x = 100;
        ecs->Create(ecs->emap.GetID("Platform14"), p);
        ecs->Create(ecs->emap.GetID("Platform14"), c);
        
        // Second floor
        p.x = 120;
        p.y = -80;
        ecs->Create(ecs->emap.GetID("Platform15"), p);
        ecs->Create(ecs->emap.GetID("Platform15"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform16"), p);
        ecs->Create(ecs->emap.GetID("Platform16"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform17"), p);
        ecs->Create(ecs->emap.GetID("Platform17"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform18"), p);
        ecs->Create(ecs->emap.GetID("Platform18"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform19"), p);
        ecs->Create(ecs->emap.GetID("Platform19"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform20"), p);
        ecs->Create(ecs->emap.GetID("Platform20"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform21"), p);
        ecs->Create(ecs->emap.GetID("Platform21"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform22"), p);
        ecs->Create(ecs->emap.GetID("Platform22"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform23"), p);
        ecs->Create(ecs->emap.GetID("Platform23"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform24"), p);
        ecs->Create(ecs->emap.GetID("Platform24"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform25"), p);
        ecs->Create(ecs->emap.GetID("Platform25"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform26"), p);
        ecs->Create(ecs->emap.GetID("Platform26"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform27"), p);
        ecs->Create(ecs->emap.GetID("Platform27"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform28"), p);
        ecs->Create(ecs->emap.GetID("Platform28"), c);
        p.x -= 20;
        ecs->Create(ecs->emap.GetID("Platform29"), p);
        ecs->Create(ecs->emap.GetID("Platform29"), c);

        // If Collided to Obstacle, return to the starting point
        p.y = 70;
        p.x = -50;
        v.y = 0;
        c.trigger = true;
        ecs->Create(ecs->emap.GetID("Obstacle1"), p);
        ecs->Create(ecs->emap.GetID("Obstacle1"), v);
        ecs->Create(ecs->emap.GetID("Obstacle1"), c);
        ecs->Create(ecs->emap.GetID("Obstacle1"), g);
        p.x = 80;
        ecs->Create(ecs->emap.GetID("Obstacle2"), p);
        ecs->Create(ecs->emap.GetID("Obstacle2"), v);
        ecs->Create(ecs->emap.GetID("Obstacle2"), c);
        ecs->Create(ecs->emap.GetID("Obstacle2"), g);
        p.x = 20;
        p.y = -40;
        ecs->Create(ecs->emap.GetID("Obstacle3"), p);
        ecs->Create(ecs->emap.GetID("Obstacle3"), v);
        ecs->Create(ecs->emap.GetID("Obstacle3"), c);
        ecs->Create(ecs->emap.GetID("Obstacle3"), g);

        p.x = -160;
        p.y = 20;
        ecs->Create(ecs->emap.GetID("Start"), p);

        p.x = -160; // for example of goal position
        p.y = -60;
        ecs->Create(ecs->emap.GetID("Goal"), p);
        ecs->Create(ecs->emap.GetID("Goal"), c);

        // Life shown (HP)
        LifeSprite ls;
        p.x = -160;
        p.y = 80;
        ecs->Create(ecs->emap.GetID("Life1"), p);
        ecs->Create(ecs->emap.GetID("Life1"), ls);
        p.x = -140;
        ecs->Create(ecs->emap.GetID("Life2"), p);
        ecs->Create(ecs->emap.GetID("Life2"), ls);
        p.x = -120;
        ecs->Create(ecs->emap.GetID("Life3"), p);
        ecs->Create(ecs->emap.GetID("Life3"), ls);

        p.x = 0;
        p.y = 0;
        ecs->Create(ecs->emap.GetID("Sign"), p);

        Health h = {3};
        HitRate r = {0};
        c.size.x = 20;
        c.size.y = 20;
        c.trigger = false;
        p.x = -160;
        p.y = 0;
        v.x = 0;
        v.y = 0;
        g.a = -1;
        ecs->Create(ecs->emap.GetID("Triceratops"), p);
        ecs->Create(ecs->emap.GetID("Triceratops"), v);
        ecs->Create(ecs->emap.GetID("Triceratops"), g);
        ecs->Create(ecs->emap.GetID("Triceratops"), c);
        ecs->Create(ecs->emap.GetID("Triceratops"), h);
        ecs->Create(ecs->emap.GetID("Triceratops"), r);
    }

    void Shutdown()
    {
        graphics->Shutdown();
        input->Shutdown();
    }

    void RunGameLoop(const UpdateCallback &callback)
    {
        //graphics->LoadOneImage("Child Brontosaurus", "assets/Child Brontosaurus face.jpg");
        while (!graphics->ShouldQuit())
        {
            std::chrono::time_point s = std::chrono::steady_clock::now();
            input->Update();
            UpdateCallback();

            graphics->Draw();
            scripts->Update();
            
            // Manage timestep
            std::chrono::time_point e = std::chrono::steady_clock::now();
            const auto one_tenth_of_a_second = std::chrono::duration<double>(1.0 / 60.0);
            std::this_thread::sleep_for(std::chrono::duration<double>(one_tenth_of_a_second - (e-s)));
        }
    }
};