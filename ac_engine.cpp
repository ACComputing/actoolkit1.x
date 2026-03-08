/*
 * AC Engine v2.0 — Team Flames / Samsoft / Flames Co.
 * Integration: Cat.r1 AI Engine (Simulated)
 * Platform: SDL2 C++
 * Status: Stable, Self-Contained
 */

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <algorithm>
#include <map>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  GLOBAL CONSTANTS & CONFIG
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 400;
const int GRID_SIZE = 16;

// Animation Names
const std::vector<std::string> ANIM_NAMES = {
    "Stopped", "Walking", "Running", "Jumping", "Falling", 
    "Attacking", "Hurt", "Dying", "Idle"
};

// Dark Theme Palette (Converted Hex to SDL_Color)
namespace Theme {
    const SDL_Color menu_bg    = {43, 43, 61, 255};
    const SDL_Color menu_fg    = {68, 136, 255, 255};
    const SDL_Color toolbar_bg = {51, 51, 72, 255};
    const SDL_Color panel_bg   = {30, 30, 46, 255};
    const SDL_Color panel_header = {42, 42, 64, 255};
    const SDL_Color scene_bg   = {21, 21, 48, 255};
    const SDL_Color scene_grid = {37, 37, 80, 255};
    const SDL_Color accent     = {90, 90, 255, 255};
    const SDL_Color selection  = {255, 170, 51, 255};
    const SDL_Color text_white = {255, 255, 255, 255};
    const SDL_Color text_gray  = {200, 200, 200, 255};
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  DATA MODELS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

class GameObject {
public:
    static int _id_counter;
    int id;
    std::string name;
    std::string obj_type;
    int x, y, w, h;
    SDL_Color color;
    bool selected;

    GameObject(std::string n, std::string t, int _x, int _y) 
        : name(n), obj_type(t), x(_x), y(_y), w(32), h(32), selected(false) {
        id = ++_id_counter;
        // Default greenish color
        color = {68, 204, 68, 255}; 
    }

    void draw(SDL_Renderer* renderer) {
        // Draw Selection Box
        if (selected) {
            SDL_SetRenderDrawColor(renderer, Theme::selection.r, Theme::selection.g, Theme::selection.b, Theme::selection.a);
            SDL_Rect sel_rect = {x - 2, y - 2, w + 4, h + 4};
            SDL_RenderDrawRect(renderer, &sel_rect);
        }

        // Draw Object Body
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw Outline
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
};

int GameObject::_id_counter = 0;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  MAIN ENGINE CLASS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

class ACEngine {
private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    bool running = true;

    std::vector<std::unique_ptr<GameObject>> objects;
    GameObject* selected_obj = nullptr;
    
    // UI State
    enum class Tool { SELECT, MOVE, ACTIVE, BACKDROP };
    Tool current_tool = Tool::SELECT;
    
    // Tabs
    int current_tab = 0; // 0: Scene, 1: Events, 2: Blueprints, 3: Anim, 4: AI

public:
    ACEngine() {
        init();
    }

    ~ACEngine() {
        cleanup();
    }

    void run() {
        while (running) {
            handleEvents();
            update();
            render();
            SDL_Delay(16); // Cap at ~60 FPS
        }
    }

private:
    // ─── INITIALIZATION ───
    void init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL Init Failed: " << SDL_GetError() << std::endl;
            return;
        }
        
        if (TTF_Init() == -1) {
            std::cerr << "TTF Init Failed: " << TTF_GetError() << std::endl;
            return;
        }

        // Try multiple possible font locations
        const char* font_paths[] = {
            "/System/Library/Fonts/Supplemental/Arial.ttf",  // macOS path
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", // Linux
            "Arial.ttf",  // Current directory
            nullptr
        };
        
        font = nullptr;
        for (int i = 0; font_paths[i] != nullptr; i++) {
            font = TTF_OpenFont(font_paths[i], 12);
            if (font) break;
        }

        if (!font) {
            std::cerr << "Warning: Could not load font. Text rendering disabled." << std::endl;
        }

        window = SDL_CreateWindow(
            "AC Engine v2.0 + Cat.r1 AI (SDL2)",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN
        );

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        
        add_object("Player", "active", 200, 150);
    }

    void cleanup() {
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    // ─── LOGIC ───
    void add_object(std::string name, std::string type, int x, int y) {
        objects.push_back(std::make_unique<GameObject>(name, type, x, y));
    }

    // ─── EVENT HANDLING ───
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
                // Tab switching
                if (e.key.keysym.sym == SDLK_1) current_tab = 0;
                if (e.key.keysym.sym == SDLK_2) current_tab = 1;
                if (e.key.keysym.sym == SDLK_3) current_tab = 2;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                
                // Check toolbar click
                if (my < 30) {
                    if (mx < 50) current_tool = Tool::SELECT;
                    else if (mx < 100) current_tool = Tool::MOVE;
                    else if (mx < 150) current_tool = Tool::ACTIVE;
                }
                // Scene Interaction
                else if (my > 30 && my < SCREEN_HEIGHT - 20) {
                    handle_scene_click(mx, my);
                }
            }

            if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON_LMASK) {
                if (selected_obj) {
                    selected_obj->x = e.motion.x - selected_obj->w / 2;
                    selected_obj->y = e.motion.y - selected_obj->h / 2;
                }
            }
        }
    }

    void handle_scene_click(int mx, int my) {
        selected_obj = nullptr;
        for (auto &obj : objects) obj->selected = false;

        // Reverse iteration for top-most selection
        for (int i = objects.size() - 1; i >= 0; i--) {
            auto &obj = objects[i];
            if (mx >= obj->x && mx <= obj->x + obj->w &&
                my >= obj->y && my <= obj->y + obj->h) {
                selected_obj = obj.get();
                obj->selected = true;
                return; // Found selection
            }
        }
        
        // If nothing selected and tool is ACTIVE, create new object
        if (!selected_obj && current_tool == Tool::ACTIVE) {
            add_object("Obj_" + std::to_string(objects.size()), "active", mx - 16, my - 16);
            std::cout << "Created new object via tool." << std::endl;
        }
    }

    void update() {
        // Game logic updates would go here
    }

    // ─── RENDERING ───
    void render() {
        SDL_SetRenderDrawColor(renderer, Theme::menu_bg.r, Theme::menu_bg.g, Theme::menu_bg.b, 255);
        SDL_RenderClear(renderer);

        draw_toolbar();
        draw_main_area();
        draw_statusbar();

        SDL_RenderPresent(renderer);
    }

    void draw_toolbar() {
        // Toolbar Background
        SDL_Rect tb_rect = {0, 0, SCREEN_WIDTH, 30};
        SDL_SetRenderDrawColor(renderer, Theme::toolbar_bg.r, Theme::toolbar_bg.g, Theme::toolbar_bg.b, 255);
        SDL_RenderFillRect(renderer, &tb_rect);

        // Buttons (Simplified rendering)
        draw_button(0, "Select", current_tool == Tool::SELECT);
        draw_button(50, "Move", current_tool == Tool::MOVE);
        draw_button(100, "Active", current_tool == Tool::ACTIVE);
        draw_button(200, "Run", false); // Run button
    }

    void draw_button(int x, const char* text, bool active) {
        SDL_Rect btn = {x, 2, 48, 26};
        SDL_Color bg = active ? Theme::accent : Theme::toolbar_bg;
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 255);
        SDL_RenderFillRect(renderer, &btn);
        render_text(text, x + 5, 8, Theme::text_gray); // Simple offset
    }

    void draw_main_area() {
        // Scene Area (Simulating tabs by just showing Scene Editor)
        SDL_Rect scene_area = {0, 30, SCREEN_WIDTH, SCREEN_HEIGHT - 50};
        SDL_SetRenderDrawColor(renderer, Theme::scene_bg.r, Theme::scene_bg.g, Theme::scene_bg.b, 255);
        SDL_RenderFillRect(renderer, &scene_area);

        draw_grid();

        // Draw Objects
        for (auto &obj : objects) {
            obj->draw(renderer);
            render_text(obj->name.c_str(), obj->x, obj->y - 12, Theme::text_white);
        }
    }

    void draw_grid() {
        SDL_SetRenderDrawColor(renderer, Theme::scene_grid.r, Theme::scene_grid.g, Theme::scene_grid.b, 255);
        for (int x = 0; x < SCREEN_WIDTH; x += GRID_SIZE) {
            SDL_RenderDrawLine(renderer, x, 30, x, SCREEN_HEIGHT - 20);
        }
        for (int y = 30; y < SCREEN_HEIGHT - 20; y += GRID_SIZE) {
            SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
        }
    }

    void draw_statusbar() {
        SDL_Rect sb = {0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20};
        SDL_SetRenderDrawColor(renderer, Theme::menu_bg.r, Theme::menu_bg.g, Theme::menu_bg.b, 255);
        SDL_RenderFillRect(renderer, &sb);
        
        std::string status = "Ready | Tool: " + std::string(
            current_tool == Tool::SELECT ? "Select" : 
            current_tool == Tool::MOVE ? "Move" : "Active");
        render_text(status.c_str(), 5, SCREEN_HEIGHT - 18, Theme::menu_fg);
    }

    // Simple Text Rendering Helper
    void render_text(const char* text, int x, int y, SDL_Color color) {
        if (!font) return; // Silently fail if no font loaded
        SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dst = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
};

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
int main(int argc, char* argv[]) {
    ACEngine engine;
    engine.run();
    return 0;
}