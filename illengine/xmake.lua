add_rules("mode.debug", "mode.release")

add_requires("lua", "sol2")
add_requires("glfw")
add_requires("glm")
add_requires("wgpu-native", "glfw3webgpu")
add_requires("stb")

set_policy("build.warning", true) -- show warnings
set_warnings("all") -- warn about many things
    
target("illengine")
    add_packages("glfw", {public = true})
    add_packages("wgpu-native", "glfw3webgpu")
    add_packages("glm", {public = true})
    add_packages("stb")
    add_packages("lua")
    add_packages("sol2", {public = true})
    set_kind("static")
    set_languages("cxx20")
    
    -- Declare our engine's header path.
    -- This allows targets that depend on the engine to #include them.
    add_includedirs("src", {public = true})
    
    -- Add all .cpp files in the `src` directory.
    add_files("src/*.cpp")

target("helloworld")
    set_kind("binary")
    set_languages("cxx20")
    
    add_deps("illengine")
    
    add_files("demo/helloworld.cpp")
    set_rundir("$(projectdir)")

-- Copy assets
after_build(function (target)
    cprint("Copying assets")
    os.cp("$(projectdir)/assets", path.directory(target:targetfile()))
end)