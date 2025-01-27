#include "GLFW/glfw3.h"
#include "GraphicsManager.h"
#include <iostream>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include <glm/mat4x4.hpp>
#include "Types.h"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "EntityComponentSystem.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

template< typename T > constexpr const T* to_ptr( const T& val ) { return &val; }
template< typename T, std::size_t N > constexpr const T* to_ptr( const T (&&arr)[N] ) { return arr; }

WGPUBuffer instance_buffer = nullptr;
WGPUBindGroup bind_group = nullptr;
WGPURenderPassEncoder render_pass = nullptr;
WGPUTextureView current_texture_view = nullptr;
WGPUCommandEncoder encoder = nullptr;
WGPUInstance instance = nullptr;
WGPUAdapter adapter = nullptr;
WGPUSurface surface = nullptr;
WGPUQueue queue = nullptr;
WGPUDevice device = nullptr;
WGPUSwapChain swapchain = nullptr;
WGPURenderPipeline pipeline = nullptr;
WGPUBuffer vertex_buffer = nullptr;
WGPUBuffer uniform_buffer = nullptr;
WGPUSampler texSampler = nullptr;
WGPUShaderModule shader_module = nullptr;

struct Image
{
    int width, height;
    WGPUTexture field = nullptr;
    WGPUTexture tex = nullptr;

    ~Image()
    {
        if(field != nullptr)
        {
            wgpuTextureDestroy(field);
            wgpuTextureRelease(field);
        }
    }
};

std::unordered_map< string, Image > map = std::unordered_map<string, Image>();

GraphicsManager::GraphicsManager(int width, int height, const char* name, bool fullscreen, EntityComponentSystem *e)
{
    GraphicsManager::window_width = width;
    GraphicsManager::window_height = height;
    GraphicsManager::window_name = name;
    GraphicsManager::window_fullscreen = fullscreen;
    ECS = e;
}

void GraphicsManager::Startup()
{
    glfwInit();
    // We don't want GLFW to set up a graphics API.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // Create the window.
    window = glfwCreateWindow(window_width, window_height, window_name, window_fullscreen ? glfwGetPrimaryMonitor() : 0, 0);
    glfwSetWindowAspectRatio(window, window_width, window_height);
    if (!window)
    {
        std::cerr << "Failed to create a window." << std::endl;
        glfwTerminate();
    }

    instance = wgpuCreateInstance(to_ptr(WGPUInstanceDescriptor{}));
    if (instance == nullptr)
    {
        std::cerr << "failed to initialize the Web GPU." << std::endl;
        glfwTerminate();
    }

    surface = glfwGetWGPUSurface(instance, window);

    adapter = nullptr;
    wgpuInstanceRequestAdapter(
        instance,
        to_ptr( WGPURequestAdapterOptions{ .compatibleSurface = surface } ),
        []( WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* adapter_ptr ) {
            if( status != WGPURequestAdapterStatus_Success ) {
                std::cerr << "Failed to get a WebGPU adapter: " << message << std::endl;
                glfwTerminate();
            }
            
            *static_cast<WGPUAdapter*>(adapter_ptr) = adapter;
        },
        &(adapter)
        );

    device = nullptr;
    wgpuAdapterRequestDevice(
        adapter,
        nullptr,
        []( WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* device_ptr ) {
            if( status != WGPURequestDeviceStatus_Success ) {
                std::cerr << "Failed to get a WebGPU device: " << message << std::endl;
                glfwTerminate();
            }
            
            *static_cast<WGPUDevice*>(device_ptr) = device;
        },
        &(device)
    );

    // An error callback to help with debugging
    wgpuDeviceSetUncapturedErrorCallback(
        device,
        []( WGPUErrorType type, char const* message, void* ) {
            std::cerr << "WebGPU uncaptured error type " << type << " with message: " << message << std::endl;
        },
        nullptr
        );

    queue = wgpuDeviceGetQueue(device);

    // A vertex buffer containing a textured square.
    const struct {
        // position
        float x, y;
        // texcoords
        float u, v;
    } vertices[] = {
        // position       // texcoords
      { -1.0f,  -1.0f,    0.0f,  1.0f },
      {  1.0f,  -1.0f,    1.0f,  1.0f },
      { -1.0f,   1.0f,    0.0f,  0.0f },
      {  1.0f,   1.0f,    1.0f,  0.0f },
    };

    vertex_buffer = wgpuDeviceCreateBuffer(device, to_ptr(WGPUBufferDescriptor{
    .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
    .size = sizeof(vertices)
        }));

    wgpuQueueWriteBuffer(queue, vertex_buffer, 0, vertices, sizeof(vertices));
    WGPUTextureFormat swap_chain_format = wgpuSurfaceGetPreferredFormat(surface, adapter);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    swapchain = wgpuDeviceCreateSwapChain( device, surface, to_ptr( WGPUSwapChainDescriptor{
        .usage = WGPUTextureUsage_RenderAttachment,
        .format = swap_chain_format,
        .width = (uint32_t)width,
        .height = (uint32_t)height
    }) );

    uniform_buffer = wgpuDeviceCreateBuffer(device, to_ptr(WGPUBufferDescriptor{
    .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
    .size = sizeof(Uniforms)
        }));

    texSampler = wgpuDeviceCreateSampler(device, to_ptr(WGPUSamplerDescriptor{
    .addressModeU = WGPUAddressMode_ClampToEdge,
    .addressModeV = WGPUAddressMode_ClampToEdge,
    .magFilter = WGPUFilterMode_Linear,
    .minFilter = WGPUFilterMode_Linear,
    .maxAnisotropy = 1
        }));

    const char* source = R"(
        struct Uniforms {
            projection: mat4x4f,
        };

        @group(0) @binding(0) var<uniform> uniforms: Uniforms;
        @group(0) @binding(1) var texSampler : sampler;
        @group(0) @binding(2) var texData : texture_2d<f32>;

        struct VertexInput {
            @location(0) position: vec2f,
                @location(1) texcoords: vec2f,
                @location(2) translation: vec3f,
                @location(3) scale: f32,
        };

        struct VertexOutput {
            @builtin(position) position: vec4f,
                @location(0) texcoords: vec2f,
        };

        @vertex
        fn vertex_shader_main(in: VertexInput) -> VertexOutput {
            var out : VertexOutput;
            out.position = uniforms.projection * vec4f(vec3f(in.scale * in.position, 0.0) + in.translation, 1.0);
            out.texcoords = in.texcoords;
            return out;
        }

        @fragment
        fn fragment_shader_main(in: VertexOutput) -> @location(0) vec4f {
            let color = textureSample(texData, texSampler, in.texcoords).rgba;
            return color;
        }
    )";

    WGPUShaderModuleWGSLDescriptor code_desc = {};
    code_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    code_desc.code = source; // The shader source as a `char*`
    WGPUShaderModuleDescriptor shader_desc = {};
    shader_desc.nextInChain = &code_desc.chain;
    shader_module = wgpuDeviceCreateShaderModule(device, &shader_desc);

    pipeline = wgpuDeviceCreateRenderPipeline( device, to_ptr( WGPURenderPipelineDescriptor{
    
    // Describe the vertex shader inputs
    .vertex = {
        .module = shader_module,
        .entryPoint = "vertex_shader_main",
        // Vertex attributes.
        .bufferCount = 2,
        .buffers = to_ptr<WGPUVertexBufferLayout>({
            // We have one buffer with our per-vertex position and UV data. This data never changes.
            // Note how the type, byte offset, and stride (bytes between elements) exactly matches our `vertex_buffer`.
            {
                .arrayStride = 4*sizeof(float),
                .attributeCount = 2,
                .attributes = to_ptr<WGPUVertexAttribute>({
                    // Position x,y are first.
                    {
                        .format = WGPUVertexFormat_Float32x2,
                        .offset = 0,
                        .shaderLocation = 0
                    },
                    // Texture coordinates u,v are second.
                    {
                        .format = WGPUVertexFormat_Float32x2,
                        .offset = 2*sizeof(float),
                        .shaderLocation = 1
                    }
                    })
            },
            // We will use a second buffer with our per-sprite translation and scale. This data will be set in our draw function.
            {
                .arrayStride = sizeof(InstanceData),
                // This data is per-instance. All four vertices will get the same value. Each instance of drawing the vertices will get a different value.
                // The type, byte offset, and stride (bytes between elements) exactly match the array of `InstanceData` structs we will upload in our draw function.
                .stepMode = WGPUVertexStepMode_Instance,
                .attributeCount = 2,
                .attributes = to_ptr<WGPUVertexAttribute>({
                    // Translation as a 3D vector.
                    {
                        .format = WGPUVertexFormat_Float32x3,
                        .offset = offsetof(InstanceData, translation),
                        .shaderLocation = 2
                    },
                    // Scale as a 2D vector for non-uniform scaling.
                    {
                        .format = WGPUVertexFormat_Float32x2,
                        .offset = offsetof(InstanceData, scale),
                        .shaderLocation = 3
                    }
                    })
            }
            })
        },
    
    // Interpret our 4 vertices as a triangle strip
    .primitive = WGPUPrimitiveState{
        .topology = WGPUPrimitiveTopology_TriangleStrip,
        },
    
    // No multi-sampling (1 sample per pixel, all bits on).
    .multisample = WGPUMultisampleState{
        .count = 1,
        .mask = ~0u
        },
    
    // Describe the fragment shader and its output
    .fragment = to_ptr( WGPUFragmentState{
        .module = shader_module,
        .entryPoint = "fragment_shader_main",
        
        // Our fragment shader outputs a single color value per pixel.
        .targetCount = 1,
        .targets = to_ptr<WGPUColorTargetState>({
            {
                .format = swap_chain_format,
                // The images we want to draw may have transparency, so let's turn on alpha blending with over compositing (ɑ⋅foreground + (1-ɑ)⋅background).
                // This will blend with whatever has already been drawn.
                .blend = to_ptr( WGPUBlendState{
                    // Over blending for color
                    .color = {
                        .operation = WGPUBlendOperation_Add,
                        .srcFactor = WGPUBlendFactor_SrcAlpha,
                        .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha
                        },
                    // Leave destination alpha alone
                    .alpha = {
                        .operation = WGPUBlendOperation_Add,
                        .srcFactor = WGPUBlendFactor_Zero,
                        .dstFactor = WGPUBlendFactor_One
                        }
                    } ),
                .writeMask = WGPUColorWriteMask_All
            }})
        } )
    } ) );
}

void GraphicsManager::Shutdown()
{
    wgpuBufferRelease(instance_buffer);
    wgpuBindGroupRelease(bind_group);
    wgpuTextureViewRelease(current_texture_view);
    wgpuCommandEncoderRelease(encoder);
    wgpuRenderPassEncoderRelease(render_pass);
    wgpuShaderModuleRelease(shader_module);
    wgpuBufferRelease(vertex_buffer);
    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuSurfaceRelease(surface);
    wgpuInstanceRelease(instance);
    glfwTerminate();
}

bool GraphicsManager::ShouldQuit()
{
    return glfwWindowShouldClose(window);
}

void GraphicsManager::SetShouldQuit(bool quit)
{
    glfwSetWindowShouldClose(window, quit);
}

bool GraphicsManager::LoadOneImage(const string& name, const string& path)
{
    int width, height, channels;
    unsigned char* data = stbi_load( path.c_str(), &width, &height, &channels, 4 );
    Image i;

    i.tex = wgpuDeviceCreateTexture( device, to_ptr( WGPUTextureDescriptor{
        .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        .dimension = WGPUTextureDimension_2D,
        .size = { (uint32_t)width, (uint32_t)height, 1 },
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1
    } ) );

    wgpuQueueWriteTexture(
        queue,
        to_ptr<WGPUImageCopyTexture>({ .texture = i.tex }),
        data,
        width * height * 4,
        to_ptr<WGPUTextureDataLayout>({ .bytesPerRow = (uint32_t)(width*4), .rowsPerImage = (uint32_t)height }),
        to_ptr( WGPUExtent3D{ (uint32_t)width, (uint32_t)height, 1 } )
    );
    i.width = width;
    i.height = height;
    map[name] = i;
    Sprite s = Sprite();
    s.image = name;
    s.size = vec2(width, height);
    EntityID id = ECS->emap.GetID(name);
    ECS->Create(id, s);

    stbi_image_free( data );

    return true;
}

void GraphicsManager::Draw()
{
    encoder = wgpuDeviceCreateCommandEncoder( device, nullptr );
    current_texture_view = wgpuSwapChainGetCurrentTextureView( swapchain );
    render_pass = wgpuCommandEncoderBeginRenderPass( encoder, to_ptr<WGPURenderPassDescriptor>({
    .colorAttachmentCount = 1,
    .colorAttachments = to_ptr<WGPURenderPassColorAttachment>({{
        .view = current_texture_view,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        // Choose the background color.
        .clearValue = WGPUColor{ 1, 1, 1, 1.0 }
        }})
    }) );
    wgpuRenderPassEncoderSetPipeline( render_pass, pipeline );
    wgpuRenderPassEncoderSetVertexBuffer( render_pass, 0, vertex_buffer, 0, 4*4*sizeof(float) );
    bind_group = nullptr;

    Uniforms uniforms;
    // Start with an identity matrix.
    uniforms.projection = mat4{1};
    // Scale x and y by 1/100.
    uniforms.projection[0][0] = uniforms.projection[1][1] = 1./100.;
    // Scale the long edge by an additional 1/(long/short) = short/long.
    if( window_width < window_height ) {
        uniforms.projection[1][1] *= window_width;
        uniforms.projection[1][1] /= window_height;
    } else {
        uniforms.projection[0][0] *= window_height;
        uniforms.projection[0][0] /= window_width;
    }
    wgpuQueueWriteBuffer( queue, uniform_buffer, 0, &uniforms, sizeof(Uniforms) );
    instance_buffer = wgpuDeviceCreateBuffer( device, to_ptr<WGPUBufferDescriptor>({
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        .size = sizeof(InstanceData) * map.size()
    }) );

    wgpuRenderPassEncoderSetVertexBuffer( render_pass, 1, instance_buffer, 0, sizeof(InstanceData) * map.size() );

    int i = 0;
    ECS->ForEach<Sprite>( [&]( EntityID entity )
    {
        Sprite& sp = ECS->Get<Sprite>(entity);
        Position& p = ECS->Get<Position>(entity);
        Image &s = map[sp.image];
        InstanceData d;
        if( sp.size.x < sp.size.y ) {
            d.scale = vec2( (sp.size.x)/sp.size.y, 1.0 );
        } else {
            d.scale = vec2( 1.0, (sp.size.y)/sp.size.x );
        }
        d.scale *= 10;
        d.translation = vec3(p.x, p.y, 0);
        wgpuQueueWriteBuffer( queue, instance_buffer, i * sizeof(InstanceData), &d, sizeof(InstanceData) );
        auto layout = wgpuRenderPipelineGetBindGroupLayout( pipeline, 0 );
        bind_group = wgpuDeviceCreateBindGroup( device, to_ptr( WGPUBindGroupDescriptor{
        .layout = layout,
        .entryCount = 3,
        // The entries `.binding` matches what we wrote in the shader.
        .entries = to_ptr<WGPUBindGroupEntry>({
            {
                .binding = 0,
                .buffer = uniform_buffer,
                .size = sizeof( Uniforms )
            },
            {
                .binding = 1,
                .sampler = texSampler,
            },
            {
                .binding = 2,
                .textureView = wgpuTextureCreateView( s.tex, nullptr )
            }
            })
        } ) );
        wgpuBindGroupLayoutRelease( layout );
        wgpuRenderPassEncoderSetBindGroup( render_pass, 0, bind_group, 0, nullptr );
        wgpuRenderPassEncoderDraw( render_pass, 4, 1, 0, i );
        ++i;
    });

    wgpuRenderPassEncoderEnd( render_pass );
    WGPUCommandBuffer command = wgpuCommandEncoderFinish( encoder, nullptr );
    wgpuQueueSubmit( queue, 1, &command );
    wgpuSwapChainPresent( swapchain );
}