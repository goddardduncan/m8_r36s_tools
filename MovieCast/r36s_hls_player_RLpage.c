// r36s_hls_player.c (scrolling + in-app log + stream control)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdarg.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define MAX_FILES 128
#define SERVER_IP "100.119.238.83"
#define SERVER_PORT 7070
#define MAX_LOG_LINES 64
#define DISPLAY_LOG_LINES 4
#define MAX_VISIBLE_FILES 17

char *file_list[MAX_FILES];
int file_count = 0;
int selected_index = 0;
int top_index = 0;
int preparing_index = -1;
pid_t ffplay_pid = -1;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
char *log_lines[MAX_LOG_LINES];
int log_line_count = 0;

void add_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *line = malloc(512);
    vsnprintf(line, 512, fmt, args);
    va_end(args);

    if (log_line_count >= MAX_LOG_LINES) {
        free(log_lines[0]);
        memmove(&log_lines[0], &log_lines[1], sizeof(char*) * (MAX_LOG_LINES - 1));
        log_line_count = MAX_LOG_LINES - 1;
    }
    log_lines[log_line_count++] = line;
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) return 0;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

void fetch_file_list() {
    CURL *curl = curl_easy_init();
    if (!curl) return;
    struct MemoryStruct chunk = {malloc(1), 0};
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/list-mp4s", SERVER_IP, SERVER_PORT);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        struct json_object *parsed_json = json_tokener_parse(chunk.memory);
        if (parsed_json && json_object_get_type(parsed_json) == json_type_array) {
            file_count = json_object_array_length(parsed_json);
            for (int i = 0; i < file_count && i < MAX_FILES; i++) {
                struct json_object *entry = json_object_array_get_idx(parsed_json, i);
                file_list[i] = strdup(json_object_get_string(entry));
            }
        }
        json_object_put(parsed_json);
    }

    free(chunk.memory);
    curl_easy_cleanup(curl);
}

void tee_up_hls_generation(const char *filename) {
    char *encoded = curl_easy_escape(NULL, filename, 0);
    if (!encoded) {
        add_log("[Tee] Failed to URL-encode filename");
        return;
    }
    char command[1024];
    snprintf(command, sizeof(command),
             "curl -s 'http://%s:%d/hls/playlist.m3u8?file=%s' >> /tmp/tee.log 2>&1",
             SERVER_IP, SERVER_PORT, encoded);

    add_log("[Tee] Running: %s", command);
    int result = system(command);
    add_log("[Tee] system() returned %d", result);

    curl_free(encoded);
}

void stop_ffplay() {
    if (ffplay_pid > 0) {
        kill(ffplay_pid, SIGTERM);
        waitpid(ffplay_pid, NULL, 0);
        ffplay_pid = -1;
        SDL_ShowWindow(window);
        add_log("[FFPLAY] Playback stopped");
    }
}

void launch_ffplay(const char *filename) {
    stop_ffplay();
    SDL_HideWindow(window);
    char *encoded = curl_easy_escape(NULL, filename, 0);
    char url[512];
    snprintf(url, sizeof(url), "http://%s:%d/hls/playlist.m3u8?file=%s", SERVER_IP, SERVER_PORT, encoded);
    curl_free(encoded);

    add_log("[FFPLAY] Launching stream: %s", url);

    ffplay_pid = fork();
    if (ffplay_pid == 0) {
        execlp("ffplay", "ffplay", "-vcodec", "h264", "-vf", "scale=640:380",
               "-reconnect", "1", "-reconnect_streamed", "1", "-reconnect_delay_max",
               "5", url, NULL);
        exit(1);
    }
}

void handle_controls(SDL_Event event) {
    if (event.type == SDL_JOYBUTTONDOWN) {
        switch (event.jbutton.button) {
            case 2:
                event.type = SDL_QUIT;
                SDL_PushEvent(&event);
                break;
            case 1:
                add_log("[UI] A pressed: %s", file_list[selected_index]);
                launch_ffplay(file_list[selected_index]);
                break;
            case 0:
                stop_ffplay();
                break;
            case 7:
                add_log("[UI] START pressed: teeing %s", file_list[selected_index]);
                tee_up_hls_generation(file_list[selected_index]);
                break;
            case 8:
                if (selected_index > 0) {
                    selected_index--;
                    if (selected_index < top_index) top_index--;
                }
                break;
            case 9:
                if (selected_index < file_count - 1) {
                    selected_index++;
                    if (selected_index >= top_index + MAX_VISIBLE_FILES) top_index++;
                }
                break;
            case 4:
                selected_index -= MAX_VISIBLE_FILES;
                if (selected_index < 0) selected_index = 0;
                top_index = selected_index;
                break;
            case 5:
                selected_index += MAX_VISIBLE_FILES;
                if (selected_index >= file_count) selected_index = file_count - 1;
                top_index = selected_index - MAX_VISIBLE_FILES + 1;
                if (top_index < 0) top_index = 0;
                break;
        }
    }
}

void render(TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255};
    SDL_Color highlight = {191, 0, 255};
    SDL_Color preparing = {255, 165, 0};

    for (int i = 0; i < MAX_VISIBLE_FILES && (top_index + i) < file_count; i++) {
        int index = top_index + i;
        SDL_Color color = (index == selected_index) ? highlight : white;
        if (index == preparing_index) color = preparing;
        char label[512];
        snprintf(label, sizeof(label), "%s%s", "  ", file_list[index]);
        SDL_Surface *surface = TTF_RenderText_Solid(font, label, color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {50, 50 + i * FONT_SIZE, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    for (int i = 0; i < DISPLAY_LOG_LINES; i++) {
        int index = log_line_count - DISPLAY_LOG_LINES + i;
        if (index < 0 || !log_lines[index]) continue;
        SDL_Surface *log_surf = TTF_RenderText_Solid(font, log_lines[index], (SDL_Color){128, 255, 128});
        SDL_Texture *log_tex = SDL_CreateTextureFromSurface(renderer, log_surf);
        SDL_Rect log_rect = {20, SCREEN_HEIGHT - (DISPLAY_LOG_LINES - i) * FONT_SIZE, log_surf->w, log_surf->h};
        SDL_RenderCopy(renderer, log_tex, NULL, &log_rect);
        SDL_FreeSurface(log_surf);
        SDL_DestroyTexture(log_tex);
    }

    SDL_RenderPresent(renderer);
}

void run_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0 || TTF_Init() < 0) return;
    window = SDL_CreateWindow("R36S Stream Browser", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        return;
    }
    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    if (!joystick) add_log("[WARN] No joystick detected");
    SDL_Event event;
    int running = 1;

    while (running) {
        render(font);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 2) running = 0; // X = quit
                else if (event.jbutton.button == 1) {
                    add_log("[UI] A pressed: %s", file_list[selected_index]);
                    launch_ffplay(file_list[selected_index]);
                }
                else if (event.jbutton.button == 0) stop_ffplay(); // B = stop
                else if (event.jbutton.button == 8 && selected_index > 0) {
                    selected_index--;
                    if (selected_index < top_index) top_index--;
                }
                else if (event.jbutton.button == 9 && selected_index < file_count - 1) {
                    selected_index++;
                    if (selected_index >= top_index + MAX_VISIBLE_FILES) top_index++;
                }
                else if (event.jbutton.button == 7) {
                    add_log("[UI] START pressed: teeing %s", file_list[selected_index]);
                    tee_up_hls_generation(file_list[selected_index]);
                    preparing_index = selected_index;
                }
                else if (event.jbutton.button == 5 && selected_index + MAX_VISIBLE_FILES < file_count) {
                    selected_index += MAX_VISIBLE_FILES;
                    top_index += MAX_VISIBLE_FILES;
                }
                else if (event.jbutton.button == 4 && selected_index >= MAX_VISIBLE_FILES) {
                    selected_index -= MAX_VISIBLE_FILES;
                    top_index -= MAX_VISIBLE_FILES;
                    if (top_index < 0) top_index = 0;
                }
            }
        }
        SDL_Delay(50);
    }

    stop_ffplay();
    for (int i = 0; i < file_count; i++) free(file_list[i]);
    for (int i = 0; i < log_line_count; i++) free(log_lines[i]);

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    fetch_file_list();
    run_ui();
    return 0;
}
