#pragma once
#include <limits>
#include <vector>
#include "inclusions.hpp" // IWYU pragma: keep

namespace CGIMBGFX {
    typedef uint8_t u8;
    typedef uint16_t u16;
    using std::numeric_limits, std::vector;

    namespace detail {
        [[nodiscard]] inline void* default_GetNativeWindowHandle(const ImGuiViewport& viewport) noexcept { return viewport.PlatformHandleRaw; }
        [[nodiscard]] inline ImVec2 default_GetFrameBufferScale(const ImGuiViewport& viewport) noexcept {
            if (viewport.FramebufferScale.x == 0.0f || viewport.FramebufferScale.y == 0.0f) return ImVec2(1.0f, 1.0f);
            return viewport.FramebufferScale;
        }
    }

    struct PlatformBridge {
        //The function to get native window handle from an `ImGuiViewport`.
        void* (*getNativeWindowHandle)(const ImGuiViewport& viewport) {detail::default_GetNativeWindowHandle};
        //The function to get the framebuffer scale.
        ImVec2 (*getFrameBufferScale)(const ImGuiViewport& viewport) {detail::default_GetFrameBufferScale};
    };

    struct BackendData {
        bx::AllocatorI* allocator{nullptr};
        bgfx::ProgramHandle shader = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
        bgfx::ViewId startingViewId{1};
        //Uses u8 instead of bool so we don't need to pay the performance cost of a dynamic bitset.
        vector<u8> viewIdUsed;
        bgfx::FrameBufferHandle renderToFrameBuffer = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout layout;
        PlatformBridge platformBridge;
    };

    [[nodiscard]] inline const BackendData* ImGui_Implbgfx_extras_GetBackendData() noexcept {
        return ImGui::GetCurrentContext() ? reinterpret_cast<BackendData*>(ImGui::GetIO().BackendRendererUserData) : nullptr;
    }

    namespace internal {
        [[nodiscard]] inline bgfx::ViewId allocateViewId() noexcept {
            auto* data = const_cast<BackendData*>(CGIMBGFX::ImGui_Implbgfx_extras_GetBackendData());
            if (data == nullptr || data->viewIdUsed.size() == 0) return bgfx::ViewId(numeric_limits<u16>::max());
            for (u16 i = 0; i < data->viewIdUsed.size(); i++) if (!data->viewIdUsed[i]) {
                data->viewIdUsed[i] = true;
                return data->startingViewId + i;
            }
            return bgfx::ViewId(numeric_limits<u16>::max());
        }

        inline void releaseViewId(bgfx::ViewId viewId) noexcept {
            auto* data = const_cast<BackendData*>(CGIMBGFX::ImGui_Implbgfx_extras_GetBackendData());
            if (data == nullptr || data->viewIdUsed.size() == 0) return;
            const u16 index = static_cast<u16>(viewId - data->startingViewId);
            if (index >= data->viewIdUsed.size() || !data->viewIdUsed[index]) return;
            data->viewIdUsed[index] = false;
        }
    }
}