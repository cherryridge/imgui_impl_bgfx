#pragma once
#include <algorithm>
#include <vector>
#include "inclusions.hpp" // IWYU pragma: keep

#include "backendData.hpp"
#include "embedded.hpp"
#include "multiViewport.hpp"

namespace CGIMBGFX {
    typedef uint8_t u8;
    typedef uint16_t u16;
    using std::clamp, std::vector;
    
    struct InitConfig {
        //Customize the allocator. Strongly not recommended. Uses bx's allocator by default.
        bx::AllocatorI* allocator{nullptr};
        //Provide your shader. The shader must support the following vertex attributes: `Position` (vec2), `TexCoord0` (vec2), `Color0` (rgba8). The shader must also support a texture sampler2D uniform. You can use your main shader or make a separate one. This is required. If you don't want to bother with writing a shader, you can set `useEmbeddedShader` to true and use the embedded shader, but that is not recommended.
        bgfx::ProgramHandle shader = BGFX_INVALID_HANDLE;
        //Provide your sampler's handle. This is required.
        bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
        //Which view ID should the backend start to use so we don't collide with your views?
        //Every view ID will be used for one ImGui viewport, which is just a platform window. The main window is the first viewport and will use the `startingViewId`. When user drags an ImGui window out of the main platform window, ImGui creates a separate viewport for it, thus consuming another view ID, and so on.
        //The default is 1 for not colliding with the backbuffer's view ID (which is often set to 0). You can make this field higher if you need more views for your own stuff, but keep in mind bgfx have a cap `BGFX_CONFIG_MAX_VIEWS` with default value of 256.
        bgfx::ViewId startingViewId{1};
        //How many view IDs should be reserved to this backend / used at maximum? The default is 16, which should be more than enough for most use cases unless you're trying to fill the desktop with ImGui windows.
        u16 maxViews{16};
        //If you want to render ImGui to a framebuffer (aka "offline rendering") other than the backbuffer (aka the main window), set this to the framebuffer handle we need to render to. By default it's invalid, which means rendering to the backbuffer.
        bgfx::FrameBufferHandle renderToFrameBuffer = BGFX_INVALID_HANDLE;
        //Enables multi viewport support. If you don't care about multi viewports, set this to false and you can ignore `platformBridge`.
        bool enableMultiViewport{true};
        //If true, the backend will use the embedded shader in the `shader` directory. This is not recommended. You should always provide your own shader and sampler handle, if possible, because that way you can have better control over the shader and you can also save some memory by sharing the shader with your own rendering code.
        bool useEmbeddedShader{false};
        //Some bridging functions for your ImGui's platform backend. If you don't need multi viewport support, set `enableMultiViewport` to false and you can ignore this. If not, refer to `README.md` for instructions on how to fill this struct.
        PlatformBridge platformBridge;
    };
    
    inline bx::DefaultAllocator defaultAllocator;
    
    [[nodiscard]] inline bool ImGui_Implbgfx_Init(const InitConfig& config) noexcept {
        IMGUI_CHECKVERSION();
        ImGuiIO& io = ImGui::GetIO();
        
    //If there is already a backend, fail.
        if (io.BackendRendererUserData != nullptr) return false;
        
    //Allocate a new backend data.
        BackendData* backendData = IM_NEW(BackendData)();
        if (backendData == nullptr) return false;
        
    //Initialize simple fields in backend data.
        backendData->allocator = config.allocator != nullptr ? config.allocator : &defaultAllocator;
        
    //Initialize handles in backend data and ownership flags.
        if (config.useEmbeddedShader) {
            if (!internal::loadEmbeddedShader(backendData->shader)) {
                IM_DELETE(backendData);
                return false;
            }
            backendData->sampler = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
            if (!bgfx::isValid(backendData->sampler)) {
                bgfx::destroy(backendData->shader);
                IM_DELETE(backendData);
                return false;
            }
        }
        else {
            backendData->shader = config.shader;
            if (!bgfx::isValid(config.shader)) {
                IM_DELETE(backendData);
                return false;
            }
            backendData->sampler = config.sampler;
            if (!bgfx::isValid(config.sampler)) {
                IM_DELETE(backendData);
                return false;
            }
        }
        
    //View ID management
        backendData->startingViewId = clamp(config.startingViewId, bgfx::ViewId(0), bgfx::ViewId(internal::U16_MAX - 1));
        backendData->viewIdUsed = vector<u8>(clamp<u16>(config.maxViews, 1, internal::U16_MAX), false);
        //Allocate the first view ID for the main viewport.
        backendData->viewIdUsed[0] = true;
        
    //Platform bridge
        backendData->platformBridge = config.platformBridge;
        
    //Create layout
        backendData->layout.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
        
    //Setup the first viewport.
        bgfx::setViewName(config.startingViewId, "ImGui Main Viewport");
        bgfx::setViewMode(config.startingViewId, bgfx::ViewMode::Sequential);
        
    //Optional rendering to frame buffer.
        if (bgfx::isValid(config.renderToFrameBuffer)) bgfx::setViewFrameBuffer(config.startingViewId, config.renderToFrameBuffer);
        
    //Multi viewport support
        if (config.enableMultiViewport) internal::initMultiViewport();
        
    //Submit the data to ImGui.
        io.BackendRendererUserData = backendData;
        io.BackendRendererName = "bgfx (CherryGrove's Implementation)";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        const bgfx::Caps* caps = bgfx::getCaps();
        if (caps != nullptr) {
            ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
            platformIo.Renderer_TextureMaxWidth = caps->limits.maxTextureSize;
            platformIo.Renderer_TextureMaxHeight = caps->limits.maxTextureSize;
        }
        
        return true;
    }
}