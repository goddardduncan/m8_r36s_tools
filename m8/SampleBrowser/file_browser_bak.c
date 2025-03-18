#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define MAX_FILES 256
#define MAX_PATH_LEN 512
#define ITEMS_PER_PAGE 17  // Max number of items visible on screen

const char *base_path = "/roms/drums/samples/";
const char *saved_path = "/roms/drums/saved/";
char current_path[MAX_PATH_LEN];

char *file_list[MAX_FILES];
int file_count = 0;
int selected_index = 0;
int top_index = 0;

SDL_Joystick *gGameController = NULL;

void log_message(const char *msg) {
    printf("%s\n", msg);
}

void enter_directory(const char *foldername) {
    if (foldername[0] == '[') {  
        char new_path[MAX_PATH_LEN];
        snprintf(new_path, sizeof(new_path), "%s/%.*s", current_path, (int)strlen(foldername) - 2, foldername + 1);

        DIR *test_dir = opendir(new_path);
        if (test_dir) {
            closedir(test_dir);
            strncpy(current_path, new_path, sizeof(current_path) - 1);
            load_directory(current_path);
        } else {
            log_message("ERROR: Cannot open selected folder.");
        }
    }
}

void ensure_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0777);
    }
}

void copy_file(const char *src, const char *dest) {
    FILE *src_file = fopen(src, "rb");
    if (!src_file) {
        log_message("ERROR: Cannot open source file.");
        return;
    }

    FILE *dest_file = fopen(dest, "wb");
    if (!dest_file) {
        log_message("ERROR: Cannot create destination file.");
        fclose(src_file);
        return;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);
    log_message("File copied successfully.");
}

void save_wav(const char *filename) {
    if (filename[0] == '[' || strcmp(filename, "...") == 0) {
        log_message("ERROR: Cannot save a folder or parent directory.");
        return;
    }

    char source_path[MAX_PATH_LEN];
    snprintf(source_path, sizeof(source_path), "%s/%s", current_path, filename);

    char destination_path[MAX_PATH_LEN];
    snprintf(destination_path, sizeof(destination_path), "%s/%s", saved_path, filename);

    ensure_directory(saved_path);
    copy_file(source_path, destination_path);
}

void load_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        log_message("ERROR: Unable to open directory.");
        return;
    }

    for (int i = 0; i < file_count; i++) free(file_list[i]);
    file_count = 0;

    if (strcmp(path, base_path) != 0) {
        file_list[file_count++] = strdup("...");
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char *folder_name = malloc(strlen(entry->d_name) + 3);
            snprintf(folder_name, strlen(entry->d_name) + 3, "[%s]", entry->d_name);
            file_list[file_count++] = folder_name;
        } else if (entry->d_type == DT_REG && strstr(entry->d_name, ".wav")) {
            file_list[file_count++] = strdup(entry->d_name);
        }
    }

    closedir(dir);
    selected_index = 0;
    top_index = 0;
}

void play_wav(const char *filename) {
    char file_path[MAX_PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/%s", current_path, filename);
    
    Mix_Chunk *sound = Mix_LoadWAV(file_path);
    if (!sound) {
        log_message("ERROR: Failed to load audio.");
        return;
    }

    Mix_PlayChannel(-1, sound, 0);
}

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, int highlight) {
    SDL_Color selectedColor = {191, 0, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, highlight ? selectedColor : color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_browser(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color textColor = {200, 200, 200};
    for (int i = 0; i < ITEMS_PER_PAGE && (top_index + i) < file_count; i++) {
        render_text(renderer, font, file_list[top_index + i], 50, 50 + i * FONT_SIZE, textColor, (top_index + i) == selected_index);
    }

    SDL_RenderPresent(renderer);
}

void run_sdl_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) return;
    if (TTF_Init() < 0) return;
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) return;

    SDL_Window *window = SDL_CreateWindow("File Browser", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);

    if (!font) return;
    if (SDL_NumJoysticks() > 0) gGameController = SDL_JoystickOpen(0);

    strncpy(current_path, base_path, sizeof(current_path) - 1);
    load_directory(current_path);

    int running = 1;
    SDL_Event event;

    while (running) {
        render_browser(renderer, font);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_JOYBUTTONDOWN) {
                switch (event.jbutton.button) {
                    case 9: if (selected_index < file_count - 1) selected_index++; break;
                    case 8: if (selected_index > 0) selected_index--; break;
                    case 5: // R1 - Page Down
                        if (selected_index + ITEMS_PER_PAGE < file_count) {
                            selected_index += ITEMS_PER_PAGE;
                            top_index += ITEMS_PER_PAGE;
                        }
                        break;
                    case 4: // L1 - Page Up
                        if (selected_index >= ITEMS_PER_PAGE) {
                            selected_index -= ITEMS_PER_PAGE;
                            top_index -= ITEMS_PER_PAGE;
                            if (top_index < 0) top_index = 0;
                        }
                        break;
                    case 1: // A Button (Select)
    if (strcmp(file_list[selected_index], "...") == 0) {
        // Go up one directory
        if (strcmp(current_path, base_path) != 0) {
            char *last_slash = strrchr(current_path, '/');
            if (last_slash && last_slash != current_path) {
                *last_slash = '\0';
            } else {
                strncpy(current_path, base_path, sizeof(current_path) - 1);
                current_path[sizeof(current_path) - 1] = '\0';
            }
            load_directory(current_path);
        }
    } else if (file_list[selected_index][0] == '[') {
        // Enter folder
        enter_directory(file_list[selected_index]);
    } else {
        // Play the selected .wav file
        play_wav(file_list[selected_index]);
    }
    break;

                    case 3: save_wav(file_list[selected_index]); break;
                    case 12: if (SDL_JoystickGetButton(gGameController, 13)) running = 0; break;
                }
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    SDL_Quit();
}

int main() { run_sdl_ui(); return 0; }
