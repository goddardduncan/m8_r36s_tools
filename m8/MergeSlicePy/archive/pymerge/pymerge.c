#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define CONSOLE_LINES 10
#define BUFFER_SIZE 256

const char *menu_items[] = {"MERGE and SLICE", "EXIT"};
int selected_index = 0;
SDL_Joystick *gGameController = NULL;
char console_output[CONSOLE_LINES][BUFFER_SIZE]; // Stores last 10 log messages
int console_lines = 0;

void log_message(const char *msg) {
    if (console_lines < CONSOLE_LINES) {
        strncpy(console_output[console_lines], msg, BUFFER_SIZE - 1);
        console_output[console_lines][BUFFER_SIZE - 1] = '\0';
        console_lines++;
    } else {
        for (int i = 1; i < CONSOLE_LINES; i++) 
            strcpy(console_output[i - 1], console_output[i]);
        strncpy(console_output[CONSOLE_LINES - 1], msg, BUFFER_SIZE - 1);
        console_output[CONSOLE_LINES - 1][BUFFER_SIZE - 1] = '\0';
    }
}

// Function to execute Python script and capture output
void run_merge_script() {
    log_message("[INFO] Running merge script...");

    FILE *fp = popen("python3 /roms/ports/merge_wav.py 2>&1", "r");
    if (fp == NULL) {
        log_message("[ERROR] Failed to run script.");
        return;
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline
        log_message(buffer);
    }

    pclose(fp);
    log_message("[INFO] Merge complete.");
}

// Rendering functions
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, int highlight) {
    SDL_Color selectedColor = {191, 0, 255}; // Purple for selected item
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, highlight ? selectedColor : color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_menu(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {200, 200, 200};
    for (int i = 0; i < 2; i++) {
        render_text(renderer, font, menu_items[i], 50, 50 + i * FONT_SIZE, textColor, i == selected_index);
    }

    // Console box
    SDL_Rect console_box = {50, 180, 540, 200};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &console_box);

    // Render logs
    for (int i = 0; i < console_lines; i++) {
        render_text(renderer, font, console_output[i], 60, 190 + i * 20, textColor, 0);
    }

    SDL_RenderPresent(renderer);
}

// Main SDL UI loop
void run_sdl_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL Initialization failed: %s\n", SDL_GetError());
        return;
    }
    if (TTF_Init() < 0) {
        printf("SDL_ttf Initialization failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Window *window = SDL_CreateWindow("Merge & Slice", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);
    if (!font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        return;
    }

    if (SDL_NumJoysticks() > 0) {
        gGameController = SDL_JoystickOpen(0);
    }

    int running = 1;
    SDL_Event event;

    while (running) {
        render_menu(renderer, font);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) 
                running = 0;

            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 9) // D-Pad Down
                    selected_index = (selected_index < 1) ? selected_index + 1 : selected_index;
                if (event.jbutton.button == 8) // D-Pad Up
                    selected_index = (selected_index > 0) ? selected_index - 1 : selected_index;
                if (event.jbutton.button == 1) { // A Button
                    if (selected_index == 0) 
                        run_merge_script();
                    if (selected_index == 1) 
                        running = 0;
                }
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    run_sdl_ui();
    return 0;
}
