#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 256
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define MAX_FILENAME_LENGTH 16
#define LOG_LINES 10 // Console log lines

const char *keyboard_layout = "QWERTYUIOP"
                              "ASDFGHJKL_"
                              "ZXCVBNM<DEL>";

int selected_index = 0;
char filename[MAX_FILENAME_LENGTH + 1] = "GB_";
int show_keyboard = 0;
SDL_Joystick *gGameController = NULL;
char log_messages[LOG_LINES][128]; // Console message buffer
int log_index = 0;
int holding_b = 0; // ✅ Track if B is being held

// ✅ Add messages to SDL2 console
void log_message(const char *msg) {
    if (log_index < LOG_LINES) {
        strncpy(log_messages[log_index], msg, 127);
        log_messages[log_index][127] = '\0';
        log_index++;
    } else {
        for (int i = 1; i < LOG_LINES; i++) strcpy(log_messages[i - 1], log_messages[i]);
        strncpy(log_messages[LOG_LINES - 1], msg, 127);
        log_messages[LOG_LINES - 1][127] = '\0';
    }
}

// ✅ Append a character to the filename
void append_character(char c) {
    if (strlen(filename) < MAX_FILENAME_LENGTH) {
        int len = strlen(filename);
        filename[len] = c;
        filename[len + 1] = '\0';
        char log[50];
        snprintf(log, 50, "Appended char: %c", c);
        log_message(log);
    }
}

// ✅ Remove last character (but keep "NAME_")
void remove_last_character() {
    int len = strlen(filename);
    if (len > 5) {
        filename[len - 1] = '\0';
        log_message("Deleted last character.");
    }
}
// Function to execute Python script and capture output
void run_merge_script() {
    log_message("[INFO] Running merge script...");

    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "python3 /roms/ports/merge_wav_name.py --prefix %s", filename);

    int result = system(command);
    if (result == -1) {
        log_message("[ERROR] Failed to run script.");
    } else {
        log_message("[INFO] Merge script executed.");
	
    }
}


// ✅ Render text
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// ✅ Render on-screen keyboard
void render_keyboard(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Color textColor = {255, 255, 255};
    SDL_Color selectedColor = {255, 0, 0};
    int startX = 100, startY = 250;
    int charIndex = 0;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 10; col++) {
            char letter[2] = {keyboard_layout[charIndex], '\0'};
            SDL_Color color = (charIndex == selected_index) ? selectedColor : textColor;
            render_text(renderer, font, letter, startX + col * 50, startY + row * 50, color);
            charIndex++;
        }
    }
}

// ✅ Render console
void render_console(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_Rect console_box = {50, 350, 540, 120};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &console_box);

    SDL_Color textColor = {200, 200, 200};
    for (int i = 0; i < log_index; i++) {
        render_text(renderer, font, log_messages[i], 60, 360 + i * 15, textColor);
    }
}

// ✅ Render everything
void render(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255};
    render_text(renderer, font, "/", 50, 50, white);
    render_text(renderer, font, filename, 200, 50, white);

    if (show_keyboard) {
        render_keyboard(renderer, font);
    }

    // render_console(renderer, font); // Show log messages

    SDL_RenderPresent(renderer);
}

// ✅ Main SDL UI loop (Hold B for keyboard, release B to select)
void run_sdl_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        log_message("SDL Init failed!");
        return;
    }
    if (TTF_Init() < 0) {
        log_message("SDL_ttf Init failed!");
        return;
    }

    SDL_Window *window = SDL_CreateWindow("Enter Name", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);
    if (!font) {
        log_message("Font loading failed!");
        return;
    }

    if (SDL_NumJoysticks() > 0) {
        gGameController = SDL_JoystickOpen(0);
        if (gGameController) {
            log_message("Joystick connected.");
        } else {
            log_message("Failed to open joystick.");
        }
    } else {
        log_message("No joystick detected.");
    }

    int running = 1;
    SDL_Event event;

    while (running) {
        render(renderer, font);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;

            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 0) { // Hold B → Show keyboard
                    show_keyboard = 1;
                    holding_b = 1;
                }
                if (event.jbutton.button == 13) { // START button
                    run_merge_script();
		   log_message("Start pressed: Saving name.");
                    running = 0;
                }
                if (event.jbutton.button == 1) remove_last_character(); // ✅ A deletes last character
                if (event.jbutton.button == 2) { // ✅ X button exits
                    log_message("X pressed: Exiting program.");
                    running = 0;
                }

                // ✅ D-Pad Buttons for Navigation
                int max_index = strlen(keyboard_layout) - 1;
                if (event.jbutton.button == 8 && selected_index >= 10) selected_index -= 10; // UP
                if (event.jbutton.button == 9 && selected_index + 10 <= max_index) selected_index += 10; // DOWN
                if (event.jbutton.button == 10 && selected_index > 0) selected_index--; // LEFT
                if (event.jbutton.button == 11 && selected_index < max_index) selected_index++; // RIGHT
            }

            if (event.type == SDL_JOYBUTTONUP && event.jbutton.button == 0) { // Release B → Select letter
                holding_b = 0;
                show_keyboard = 0;
                char selected_char = keyboard_layout[selected_index];
                if (selected_char == '_') append_character(' ');
                else if (selected_char == '<') remove_last_character();
                else append_character(selected_char);
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
