#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 18
#define MAX_LINES 15

SDL_Joystick *gGameController = NULL;
char logs[MAX_LINES][256];
int log_count = 0;

// Run a system command and capture output
void run_command(const char *command, const char *label) {
    FILE *fp = popen(command, "r");
    if (!fp) {
        snprintf(logs[log_count++], 256, "Failed: %s", command);
        return;
    }

    snprintf(logs[log_count++], 256, "--- %s ---", label);
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) && log_count < MAX_LINES - 1) { // Prevent overflow
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline
        snprintf(logs[log_count], sizeof(logs[log_count]), "%s", buffer); // ✅ Safe copying
        log_count++; 
    }

    pclose(fp);
}

// Save logs to a file
void save_log_file() {
    FILE *file = fopen("usb_test.log", "w");
    if (!file) return;

    for (int i = 0; i < log_count; i++) {
        fprintf(file, "%s\n", logs[i]);
    }
    fclose(file);
}

// Render text on SDL screen
void render_text(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color color = {200, 200, 200, 255};  // Alpha (a) value added

    for (int i = 0; i < log_count; i++) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, logs[i], color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {20, 20 + i * 20, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

// SDL Main Loop
void run_sdl_ui() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    TTF_Init();

    SDL_Window *window = SDL_CreateWindow("USB Diagnostic", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);

    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }

    // Run diagnostic commands
    run_command("mkdir -p /USB", "USB folder ready");
    run_command("sudo mount -t vfat /dev/sda1 /USB", "Mounting USB Drive");
    run_command("lsblk", "Block Devices");
    run_command("lsusb", "USB Devices");
    run_command("dmesg | tail -20", "Kernel Messages");
    run_command("ls /dev/sd*", "Storage Devices");
run_command("arecord -l");
run_command("cat /proc/asound/cards");



    save_log_file(); // Save results to file

    int running = 1;
    SDL_Event event;

    while (running) {
        render_text(renderer, font);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_JOYBUTTONDOWN && event.jbutton.button == 1) running = 0;  // Press "A" to exit
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
