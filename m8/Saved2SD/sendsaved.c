#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MAX_PATH_LEN 512
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24

const char *menu_items[] = {"MOUNT USB", "SEND DRUMSAMPLES TO SD/SAVED", "EXIT"};
int selected_index = 0;
SDL_Joystick *gGameController = NULL;
char console_output[10][64]; // Stores last 10 log messages
int console_lines = 0;

void log_message(const char *msg) {
    if (console_lines < 10) {
        strncpy(console_output[console_lines], msg, 63);
        console_output[console_lines][63] = '\0';
        console_lines++;
    } else {
        for (int i = 1; i < 10; i++) strcpy(console_output[i - 1], console_output[i]);
        strncpy(console_output[9], msg, 63);
        console_output[9][63] = '\0';
    }
}

// Function to clean up USB mount point (only after successful unmount)
void clean_mount_point() {
    log_message("[INFO] Cleaning up /USB...");
    system("sudo rm -rf /USB/*");
}

// Function to mount USB
void mount_usb() {
    log_message("[INFO] Mounting USB...");

    

    // Unmount first to prevent conflicts
    system("sudo umount /USB 2>/dev/null");
    // Ensure the mount point exists
    system("sudo mkdir -p /USB");
    system("sudo rm -rf /USB/*");	

    // Run mount command and capture output
    int mount_status = system("sudo mount -o defaults,sync,noatime /dev/sda1 /USB 2>&1 | tee /tmp/mount_log.txt");

    if (mount_status == 0 && access("/USB", W_OK) == 0) {
        log_message("[SUCCESS] USB Mounted Successfully!");
        system("sudo chmod -R 777 /USB/samples/Sliced");
    } else {
        log_message("[ERROR] USB Mount Failed! Check /tmp/mount_log.txt");
        system("cat /tmp/mount_log.txt | tee -a /tmp/usb_error_log.txt");
    }
}


void unmount_usb() {
    log_message("[INFO] Unmounting USB...");
    int unmount_status = system("sudo umount /USB 2>/tmp/unmount_log.txt");

    if (unmount_status == 0) {
        log_message("[SUCCESS] USB Unmounted.");
        clean_mount_point();
    } else {
        log_message("[ERROR] USB Unmount Failed! Check /tmp/unmount_log.txt");
        system("cat /tmp/unmount_log.txt | tee -a /tmp/usb_error_log.txt");
    }
}


// Function to transfer drum samples
void send_drums_to_sd() {
    log_message("[INFO] Starting file transfer...");
    
    system("sudo mkdir -p /USB/samples/saved");  // Ensure destination exists
    system("sudo chmod 777 /USB/samples/saved"); // Ensure it's writable

    if (access("/USB/samples/saved", W_OK) != 0) {
        log_message("[ERROR] /USB/samples/saved is not writable!");
        return;
    }

    DIR *dir = opendir("/roms/drums/saved");
    if (!dir) {
        log_message("[ERROR] Source folder not found!");
        return;
    }

    struct dirent *entry;
    char src_path[MAX_PATH_LEN], dest_path[MAX_PATH_LEN];

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG || !strstr(entry->d_name, ".wav")) continue;

        snprintf(src_path, sizeof(src_path), "/roms/drums/saved/%s", entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "/USB/samples/saved/%s", entry->d_name);

        if (access(dest_path, F_OK) == 0) {
            char skip_msg[64];
            snprintf(skip_msg, sizeof(skip_msg), "Skipping: %.50s...", entry->d_name);
            log_message(skip_msg);
            continue;
        }

        FILE *src = fopen(src_path, "rb");
        if (!src) {
            log_message("[ERROR] Failed to open source file!");
            continue;
        }

        FILE *dest = fopen(dest_path, "wb");
        if (!dest) {
            char error_msg[128];
            snprintf(error_msg, sizeof(error_msg), "[ERROR] Can't open %.80s (%s)", dest_path, strerror(errno));
            log_message(error_msg);
            fclose(src);
            continue;
        }

        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0)
            fwrite(buffer, 1, bytes, dest);

        fclose(src);
        fclose(dest);

        char copy_msg[64];
        snprintf(copy_msg, sizeof(copy_msg), "Copied: %.50s...", entry->d_name);
        log_message(copy_msg);
    }

    closedir(dir);

    log_message("[INFO] File transfer complete.");
    
    // Unmount after transfer
    unmount_usb();
}

// Rendering functions (Restored Console Output)
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
    for (int i = 0; i < 3; i++) {
        render_text(renderer, font, menu_items[i], 50, 50 + i * FONT_SIZE, textColor, i == selected_index);
    }

    // Console box (Restored)
    SDL_Rect console_box = {50, 180, 540, 200};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &console_box);

    // Render logs
    for (int i = 0; i < console_lines; i++) {
        render_text(renderer, font, console_output[i], 60, 190 + i * 20, textColor, 0);
    }

    SDL_RenderPresent(renderer);
}

// Main SDL UI loop (Fully Restored Controls)
void run_sdl_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL Initialization failed: %s\n", SDL_GetError());
        return;
    }
    if (TTF_Init() < 0) {
        printf("SDL_ttf Initialization failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Window *window = SDL_CreateWindow("USB Manager", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
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
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 9) selected_index = (selected_index < 2) ? selected_index + 1 : selected_index;
                if (event.jbutton.button == 8) selected_index = (selected_index > 0) ? selected_index - 1 : selected_index;
                if (event.jbutton.button == 1) {
                    if (selected_index == 0) mount_usb();
                    if (selected_index == 1) send_drums_to_sd();
                    if (selected_index == 2) running = 0;
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
