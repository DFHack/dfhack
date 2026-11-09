// SPDX-License-Identifier: Zlib
// Optional desktop/GPU regression. Link SDL2, put the game's SDL2 runtime next
// to this executable, and run it separately from DF. No game/save is loaded.
#define SDL_MAIN_HANDLED

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL.h>

#include "worldmap-zoom-canvas.h"
#include "worldmap-zoom-sdl.h"

static WorldMapSdl api() {
    WorldMapSdl s;
    s.get_scale = SDL_RenderGetScale;
    s.set_scale = SDL_RenderSetScale;
    s.get_viewport = SDL_RenderGetViewport;
    s.set_viewport = SDL_RenderSetViewport;
    s.get_clip = SDL_RenderGetClipRect;
    s.set_clip = SDL_RenderSetClipRect;
    s.clip_enabled = SDL_RenderIsClipEnabled;
    s.renderer_info = SDL_GetRendererInfo;
    s.get_target = SDL_GetRenderTarget;
    s.set_target = SDL_SetRenderTarget;
    s.create_texture = SDL_CreateTexture;
    s.destroy_texture = SDL_DestroyTexture;
    s.texture_blend = SDL_SetTextureBlendMode;
    s.texture_scale = SDL_SetTextureScaleMode;
    s.get_color = SDL_GetRenderDrawColor;
    s.set_color = SDL_SetRenderDrawColor;
    s.clear = SDL_RenderClear;
    s.fill = SDL_RenderFillRect;
    s.copy = SDL_RenderCopy;
    return s;
}

static void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(std::string(message) + ": " + SDL_GetError());
}

static void draw_map(SDL_Renderer *renderer) {
    // Distinct, opaque 16-pixel cells make the expected magnified image exact.
    for (int row = 0; row < 24; ++row) {
        for (int col = 0; col < 32; ++col) {
            check(SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(20 + col * 7 % 200),
                                         static_cast<Uint8>(20 + row * 11 % 200),
                                         static_cast<Uint8>(90 + (row + col) % 100), SDL_ALPHA_OPAQUE) == 0,
                  "draw colour");
            SDL_Rect cell{col * 16, row * 16, 16, 16};
            check(SDL_RenderFillRect(renderer, &cell) == 0, "draw map cell");
        }
    }
}

static void test_driver(SDL_Renderer *renderer, SDL_Window *window, const char *name, int zoom) {
    auto sdl = api();
    worldmap_zoom::MapCanvas<WorldMapSdl> canvas(sdl);
    check(SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) == 0, "background");
    check(SDL_RenderClear(renderer) == 0, "clear window");
    draw_map(renderer);
    const SDL_Rect ui{560, 20, 60, 30};
    check(SDL_SetRenderDrawColor(renderer, 240, 30, 30, SDL_ALPHA_OPAQUE) == 0, "baseline UI colour");
    check(SDL_RenderFillRect(renderer, &ui) == 0, "baseline UI");
    // Compare against native drawing, not nominal RGB constants: an SDL
    // backend can encode the output framebuffer (e.g. sRGB on OpenGL ES 2).
    std::vector<uint8_t> baseline(640 * 480 * 4);
    check(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, baseline.data(), 640 * 4) == 0,
          "read native baseline");
    check(SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) == 0, "reset background");
    check(SDL_RenderClear(renderer) == 0, "reset window");
    const SDL_Rect map{0, 0, 512, 384};
    check(canvas.begin(renderer, SDL_GetWindowID(window), map,
                       worldmap_zoom::CanvasView{static_cast<double>(zoom), 4, 3, 1}, 1, true),
          "begin production canvas");
    draw_map(renderer);
    canvas.markFullDraw();
    canvas.finish(true, [&] { draw_map(renderer); });
    check(canvas.error().empty(), canvas.error().c_str());
    check(canvas.stats().restore_errors == 0 && canvas.stats().composites == 1, "canvas state");
    // Draw native-size UI *after* composition, as DF's renderer does.
    check(SDL_SetRenderDrawColor(renderer, 240, 30, 30, SDL_ALPHA_OPAQUE) == 0, "UI colour");
    check(SDL_RenderFillRect(renderer, &ui) == 0, "draw UI");
    std::vector<uint8_t> pixels(640 * 480 * 4);
    check(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), 640 * 4) == 0,
          "read composed pixels");
    for (int y = 0; y < 384; ++y) {
        for (int x = 0; x < 512; ++x) {
            const auto *pixel = pixels.data() + (y * 640 + x) * 4;
            const auto *native = baseline.data() + (((3 * 16 + y / zoom) * 640) + 4 * 16 + x / zoom) * 4;
            check(pixel[0] == native[0] && pixel[1] == native[1] && pixel[2] == native[2],
                  "magnified map pixel mismatch");
        }
    }
    for (int y = 0; y < 480; ++y) {
        for (int x = 512; x < 640; ++x) {
            const auto offset = (y * 640 + x) * 4;
            const auto *pixel = pixels.data() + offset;
            const auto *native = baseline.data() + offset;
            check(pixel[0] == native[0] && pixel[1] == native[1] && pixel[2] == native[2],
                  "native-size UI pixel mismatch");
        }
    }
    canvas.release(renderer, SDL_GetWindowID(window));
    std::cout << "PASS: " << name << ' ' << zoom << "x map pixels and native-size UI\n";
}

int main(int argc, char **argv) {
    try {
        check(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL video init");
        bool direct3d = false;
        int tested = 0;
        const std::string driver_filter = argc > 2 && std::string(argv[1]) == "--driver" ? argv[2] : "";
        for (int index = 0; index < SDL_GetNumRenderDrivers(); ++index) {
            SDL_RendererInfo info{};
            check(SDL_GetRenderDriverInfo(index, &info) == 0, "renderer info");
            if (!driver_filter.empty() && driver_filter != info.name)
                continue;
            std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
                SDL_CreateWindow("Map canvas regression", 0, 0, 640, 480, SDL_WINDOW_HIDDEN),
                SDL_DestroyWindow);
            check(window != nullptr, "create hidden window");
            std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
                SDL_CreateRenderer(window.get(), index, SDL_RENDERER_TARGETTEXTURE), SDL_DestroyRenderer);
            if (!renderer) {
                std::cout << "SKIP: " << info.name << '\n';
                continue;
            }
            for (int zoom : {2, 4})
                test_driver(renderer.get(), window.get(), info.name, zoom);
            direct3d = direct3d || std::string(info.name) == "direct3d";
            ++tested;
        }
        check(tested > 0, "no available renderer");
        if (argc > 1 && std::string(argv[1]) == "--require-direct3d")
            check(direct3d, "Direct3D regression was not exercised");
        SDL_Quit();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        SDL_Quit();
        return 1;
    }
}
