#define _XOPEN_SOURCE 500
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <ftw.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define MAX_LINES 10
#define MAX_LINE_LEN 64

int selected_index = 0;
char console_output[MAX_LINES][MAX_LINE_LEN];
int console_lines = 0;
SDL_Joystick *gGameController = NULL;
SDL_Renderer *gRenderer = NULL;
TTF_Font *gFont = NULL;

void render_console();
void log_message(const char *msg);  // forward declaration


const char *menu_items[] = {
    "MOUNT USB",
    "COPY RENDERS & SONGS",
    "UNMOUNT USB",
    "UPLOAD TO GITHUB",
    "EXIT"
};

void log_message(const char *msg) {
    if (console_lines < MAX_LINES) {
        strncpy(console_output[console_lines], msg, MAX_LINE_LEN - 1);
        console_output[console_lines][MAX_LINE_LEN - 1] = '\0';
        console_lines++;
    } else {
        for (int i = 1; i < MAX_LINES; i++) strcpy(console_output[i - 1], console_output[i]);
        strncpy(console_output[MAX_LINES - 1], msg, MAX_LINE_LEN - 1);
        console_output[MAX_LINES - 1][MAX_LINE_LEN - 1] = '\0';
    }

    render_console(); // Force screen update right after logging
}


int convert_wav_to_mp3(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F && strstr(fpath, ".wav")) {
        char mp3_path[512], cmd[1024];

        strncpy(mp3_path, fpath, sizeof(mp3_path));
        mp3_path[strlen(mp3_path) - 4] = '\0'; // remove ".wav"
        strncat(mp3_path, ".mp3", sizeof(mp3_path) - strlen(mp3_path) - 1);

        snprintf(cmd, sizeof(cmd), "ffmpeg -y -loglevel error -i '%s' '%s'", fpath, mp3_path);

        // ✨ Add this log
        const char *filename = strrchr(fpath, '/');
	filename = filename ? filename + 1 : fpath;  // strip directory if present

	char logbuf[256];
	snprintf(logbuf, sizeof(logbuf), "[INFO] Converting: %s → %s", filename, strrchr(mp3_path, '/') ? strrchr(mp3_path, '/') + 1 : mp3_path);
	log_message(logbuf);


        int c = system(cmd);
        if (c == 0) {
            log_message("[INFO] MP3 created.");
            unlink(fpath);
            log_message("[INFO] WAV removed.");
        } else {
            log_message("[ERROR] Conversion failed.");
        }
    }
    return 0;
}



void mount_usb() {
    log_message("[INFO] Mounting USB...");
    system("sudo umount /USB 2>/dev/null");
    system("sudo mkdir -p /USB");
    int status = system("sudo mount -o defaults,sync,noatime /dev/sda1 /USB");
    if (status == 0 && access("/USB", W_OK) == 0) {
        log_message("[SUCCESS] USB Mounted.");
    } else {
        log_message("[ERROR] Mount Failed.");
    }
}

void convert_all_renders_recursively() {
    log_message("[INFO] Converting all renders to MP3...");
    nftw("/roms/drums/renders", convert_wav_to_mp3, 10, FTW_PHYS);
}

void unmount_usb() {
    log_message("[INFO] Unmounting USB...");
    int status = system("sudo umount /USB");
    if (status == 0) {
        log_message("[SUCCESS] USB Unmounted.");
        system("sudo rm -rf /USB/*");
    } else {
        log_message("[ERROR] Unmount Failed.");
    }
}

void copy_renders_songs() {
    log_message("[INFO] Copying files...");
    system("sudo mkdir -p /roms/drums/renders /roms/drums/songs");
    system("sudo chmod -R 777 /roms/drums/renders /roms/drums/songs");

    int r1 = system("sudo cp -r /USB/renders/. /roms/drums/renders/ 2>/dev/null");
    int r2 = system("sudo cp -r /USB/songs/. /roms/drums/songs/ 2>/dev/null");
    if (r1 == 0 || r2 == 0) {
        log_message("[SUCCESS] Files copied.");
    } else {
        log_message("[ERROR] Copy failed.");
        return;
    }

    log_message("[INFO] Converting all WAVs to MP3...");
    nftw("/roms/drums/renders", convert_wav_to_mp3, 10, FTW_PHYS);
    log_message("[INFO] Unmounting USB...");
    int status = system("sudo umount /USB");
    if (status == 0) {
        log_message("[SUCCESS] USB Unmounted.");
        system("sudo rm -rf /USB/*");
    } else {
        log_message("[ERROR] Unmount Failed.");
    }
}

void upload_to_github() {
    log_message("[INFO] Initiate GitHub upload.");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    char zip_cmd[256];
    snprintf(zip_cmd, sizeof(zip_cmd), "cd /roms/drums && zip -r backups/backup_%s.zip songs renders", timestamp);

    int z = system("sudo mkdir -p /roms/drums/backups && sudo chmod 777 /roms/drums/backups");
    z += system(zip_cmd);
    if (z != 0) {
        log_message("[ERROR] Zip failed.");
        return;
    }

    char git_cmds[] =
        "cd /roms/drums/backups && "
        "git config --global user.email 'goddardduncan@gmail.com' && "
        "git config --global user.name 'goddardduncan' && "
        "git init && "
        "(git remote | grep origin || git remote add origin [TOKEN]@github.com/goddardduncan/m8songs.git) && "
        "git add . && git commit -m 'Auto Backup' && git push -u origin master --force";

    int g = system(git_cmds);
    if (g == 0) {
        log_message("[SUCCESS] Upload complete.");
    } else {
        log_message("[ERROR] Upload failed.");
    }
}

void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, int highlight) {
    SDL_Color highlightColor = {191, 0, 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, highlight ? highlightColor : color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void render_menu(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color color = {200, 200, 200};
    for (int i = 0; i < 5; i++) {
        render_text(renderer, font, menu_items[i], 50, 50 + i * FONT_SIZE, color, i == selected_index);
    }

    SDL_Rect console_box = {50, 180, 540, 200};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &console_box);

    for (int i = 0; i < console_lines; i++) {
        render_text(renderer, font, console_output[i], 60, 190 + i * 20, color, 0);
    }

    SDL_RenderPresent(renderer);
}
void render_console() {
    if (!gRenderer || !gFont) return;

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

    SDL_Color color = {200, 200, 200};

    for (int i = 0; i < 5; i++) {
        render_text(gRenderer, gFont, menu_items[i], 50, 50 + i * FONT_SIZE, color, i == selected_index);
    }

    SDL_Rect console_box = {50, 180, 540, 200};
    SDL_SetRenderDrawColor(gRenderer, 50, 50, 50, 255);
    SDL_RenderFillRect(gRenderer, &console_box);

    for (int i = 0; i < console_lines; i++) {
        render_text(gRenderer, gFont, console_output[i], 60, 190 + i * 20, color, 0);
    }

    SDL_RenderPresent(gRenderer);

    SDL_PumpEvents();    // ← Ensures SDL refreshes screen immediately
    SDL_Delay(10);       // ← Optional, lets screen "breathe"
}


void run_menu() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0 || TTF_Init() < 0) {
        printf("SDL Init Error\n");
        return;
    }

    SDL_Window *win = SDL_CreateWindow("USB Manager", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);

// ✅ Set globals so render_console() works
gRenderer = ren;
gFont = font;

    if (!font) return;

    if (SDL_NumJoysticks() > 0) gGameController = SDL_JoystickOpen(0);

    int running = 1;
    SDL_Event e;
    while (running) {
        render_menu(ren, font);
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_JOYBUTTONDOWN) {
                switch (e.jbutton.button) {
                    case 9: selected_index = (selected_index + 1) % 5; break; // down
                    case 8: selected_index = (selected_index - 1 + 5) % 5; break; // up
                    case 1:
                        switch (selected_index) {
                            case 0: mount_usb(); break;
                            case 1: copy_renders_songs(); break;
                            case 2: unmount_usb(); break;
                            case 3: upload_to_github(); break;
                            case 4: running = 0; break;
                        }
                        break;
                    case 2: case 13: case 12: running = 0; break; // X, Start, Select = exit
                }
            }
        }
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_CloseFont(font);
    SDL_Quit();
}

int main() {
    run_menu();
    return 0;
}
