#pragma once
#include <string>
#include <bgfx/bgfx.h>
#include <bx/bx.h>
#include <imgui.h>

namespace CherryGrove_ImGui_Impl_bgfx {
    using std::string;

    struct Data {
        string viewName;
        bx::AllocatorI* allocator{nullptr};
        bgfx::ViewId viewId;
        bgfx::ProgramHandle shader = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle frameBuffer = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout layout;
    };

    [[nodiscard]] inline Data* ImGui_ImplBgfx_internal_GetBackendData() noexcept {
        return ImGui::GetCurrentContext() ? reinterpret_cast<Data*>(ImGui::GetIO().BackendRendererUserData) : nullptr;
    }
}