#pragma once
#include <string_view>
#include <bgfx/bgfx.h>
#include <bx/allocator.h>
#include <bx/bx.h>
#include <imgui.h>

#include "data.hpp"

namespace CherryGrove_ImGui_Impl_bgfx {
    typedef uint8_t u8;
    using std::string_view;

    struct ImGui_Implbgfx_Init_Config {
        string_view viewName{"ImGui View"};
        bx::AllocatorI* allocator{nullptr};
        bgfx::ViewId viewId;
        bgfx::ProgramHandle shader = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle frameBuffer = BGFX_INVALID_HANDLE;
    };

    inline bx::DefaultAllocator allocator;

    [[nodiscard]] inline bool ImGui_Implbgfx_Init(const ImGui_Implbgfx_Init_Config& config) noexcept {
        IMGUI_CHECKVERSION();
        ImGuiIO& io = ImGui::GetIO();

    //If there is already a backend, fail.
        if (io.BackendRendererUserData == nullptr) return false;

    //Allocate a new backend data.
        Data* backendData = IM_NEW(Data)();
        if (backendData == nullptr) return false;

    //Initialize simple fields in backend data.
        backendData->viewName = config.viewName;
        backendData->allocator = config.allocator != nullptr ? config.allocator : &allocator;
        backendData->viewId = config.viewId;

    //Initialize handles in backend data and ownership flags.
        backendData->shader = config.shader;
        //Yes, we decided to require shader and sampler handles.
        if (!bgfx::isValid(config.shader)) return false;
        backendData->sampler = config.sampler;
        if (!bgfx::isValid(config.sampler)) return false;

    //Submit the first stage's data to ImGui.
        io.BackendRendererUserData = backendData;
        io.BackendRendererName = "ImGui CherryGrove Impl for bgfx";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
        const bgfx::Caps* caps = bgfx::getCaps();
        if (caps != nullptr) {
            ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
            platformIo.Renderer_TextureMaxWidth = caps->limits.maxTextureSize;
            platformIo.Renderer_TextureMaxHeight = caps->limits.maxTextureSize;
        }

        const auto failAndCleanup = [&]() noexcept {
            io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
            io.BackendRendererName = nullptr;
            io.BackendRendererUserData = nullptr;
            IM_DELETE(backendData);
            return false;
        };

        backendData->layout.begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

        bgfx::setViewName(backendData->viewId, backendData->viewName.c_str());
        bgfx::setViewMode(backendData->viewId, bgfx::ViewMode::Sequential);

        if (bgfx::isValid(config.frameBuffer)) bgfx::setViewFrameBuffer(backendData->viewId, config.frameBuffer);

        return true;
    }
}