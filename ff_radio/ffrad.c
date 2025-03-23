#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define MAX_CHANNELS 24
#define CHANNELS_FILE "/roms/tvradio/channelsRadio.json"

const char *channels[MAX_CHANNELS];
const char *urls[MAX_CHANNELS];
int channel_count = 0;

int selected_channel = 0;
pid_t ffplay_pid = -1;
int streaming = 0;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void stop_ffplay() {
    if (ffplay_pid > 0) {
        printf("Stopping ffplay (PID: %d)\n", ffplay_pid);
        fflush(stdout);
        kill(ffplay_pid, SIGTERM);
        sleep(1);
        if (waitpid(ffplay_pid, NULL, WNOHANG) == 0) {
            printf("ffplay did not exit, sending SIGKILL\n");
            kill(ffplay_pid, SIGKILL);
        }
        waitpid(ffplay_pid, NULL, 0);
        ffplay_pid = -1;
        streaming = 0;
        SDL_ShowWindow(window);
    }
}

void launch_ffplay(const char *url) {
    stop_ffplay();
    SDL_HideWindow(window);
    ffplay_pid = fork();
    if (ffplay_pid == 0) {
        execlp("ffplay", "ffplay", "-nodisp", "-autoexit", url, NULL);
        exit(1);
    }
    streaming = 1;
}

void render(SDL_Renderer *renderer, TTF_Font *font) {
    if (streaming) return;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255};
    SDL_Color purple = {128, 0, 128};
    SDL_Color blue = {0, 0, 255};

    int start_index = selected_channel - 4;
    if (start_index < 0) start_index = 0;
    int end_index = start_index + 9;
    if (end_index >= channel_count) end_index = channel_count - 1;

    for (int i = start_index; i <= end_index; i++) {
        SDL_Color color = (i == selected_channel) ? (streaming ? blue : purple) : white;
        SDL_Surface *surface = TTF_RenderText_Solid(font, channels[i], color);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {50, (SCREEN_HEIGHT / 2) + (i - selected_channel) * 30, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }
    SDL_RenderPresent(renderer);
}

int load_channels(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    char *json_data = malloc(fsize + 1);
    fread(json_data, 1, fsize, fp);
    json_data[fsize] = 0;
    fclose(fp);

    struct json_object *parsed_json = json_tokener_parse(json_data);
    free(json_data);
    if (!parsed_json) return 0;

    channel_count = 0;
    json_object_object_foreach(parsed_json, key, val) {
        if (channel_count >= MAX_CHANNELS) break;
        channels[channel_count] = strdup(key);
        urls[channel_count] = strdup(json_object_get_string(val));
        channel_count++;
    }
    json_object_put(parsed_json);
    return 1;
}

void run_sdl_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) return;
    if (TTF_Init() < 0) return;

    window = SDL_CreateWindow("Radio Selector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);

    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    int running = 1, start_pressed = 0, select_pressed = 0;
    SDL_Event event;

    while (running) {
        render(renderer, font);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 2) { stop_ffplay(); running = 0; }
                else if (!streaming && event.jbutton.button == 1) launch_ffplay(urls[selected_channel]);
                else if (streaming && event.jbutton.button == 0) stop_ffplay();
                else if (!streaming && event.jbutton.button == 8 && selected_channel > 0) selected_channel--;
                else if (!streaming && event.jbutton.button == 9 && selected_channel < channel_count - 1) selected_channel++;
                else if (event.jbutton.button == 12) start_pressed = 1;
                else if (event.jbutton.button == 13) select_pressed = 1;
                if (!streaming && start_pressed && select_pressed) running = 0;
            }
            if (event.type == SDL_JOYBUTTONUP) {
                if (event.jbutton.button == 12) start_pressed = 0;
                if (event.jbutton.button == 13) select_pressed = 0;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    if (!load_channels(CHANNELS_FILE)) {
        printf("Failed to load %s\n", CHANNELS_FILE);
        return 1;
    }
    run_sdl_ui();
    return 0;
}