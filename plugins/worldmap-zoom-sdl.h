// SPDX-License-Identifier: Zlib
#pragma once

#include <SDL.h>

// Bind to the SDL instance DF already owns; never load/link a second runtime.
struct WorldMapSdl {
    decltype(&SDL_ThreadID) thread_id = nullptr;
    decltype(&SDL_RenderGetScale) get_scale = nullptr;
    decltype(&SDL_RenderSetScale) set_scale = nullptr;
    decltype(&SDL_RenderGetViewport) get_viewport = nullptr;
    decltype(&SDL_RenderSetViewport) set_viewport = nullptr;
    decltype(&SDL_RenderGetClipRect) get_clip = nullptr;
    decltype(&SDL_RenderSetClipRect) set_clip = nullptr;
    decltype(&SDL_RenderIsClipEnabled) clip_enabled = nullptr;
    decltype(&SDL_RenderReadPixels) read_pixels = nullptr;
    decltype(&SDL_GetRendererOutputSize) output_size = nullptr;
    decltype(&SDL_SetWindowSize) window_size = nullptr;
    decltype(&SDL_SetWindowFullscreen) fullscreen = nullptr;
    decltype(&SDL_GetWindowDisplayIndex) display_index = nullptr;
    decltype(&SDL_GetDesktopDisplayMode) desktop_mode = nullptr;
    decltype(&SDL_GetWindowID) window_id = nullptr;
    decltype(&SDL_GetKeyboardFocus) keyboard_focus = nullptr;
    decltype(&SDL_AddEventWatch) add_event_watch = nullptr;
    decltype(&SDL_DelEventWatch) del_event_watch = nullptr;
    decltype(&SDL_GetRendererInfo) renderer_info = nullptr;
    decltype(&SDL_GetRenderTarget) get_target = nullptr;
    decltype(&SDL_SetRenderTarget) set_target = nullptr;
    decltype(&SDL_CreateTexture) create_texture = nullptr;
    decltype(&SDL_DestroyTexture) destroy_texture = nullptr;
    decltype(&SDL_SetTextureBlendMode) texture_blend = nullptr;
    decltype(&SDL_SetTextureScaleMode) texture_scale = nullptr;
    decltype(&SDL_GetRenderDrawColor) get_color = nullptr;
    decltype(&SDL_SetRenderDrawColor) set_color = nullptr;
    decltype(&SDL_RenderClear) clear = nullptr;
    decltype(&SDL_RenderFillRect) fill = nullptr;
    decltype(&SDL_RenderCopy) copy = nullptr;
};
