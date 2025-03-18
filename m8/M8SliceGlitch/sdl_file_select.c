#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MAX_FILES 100
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define DOWNLOAD_PATH "/roms/drums/sliced/"

char *file_list[MAX_FILES]; // Store fetched filenames
int file_count = 0;
int selected_index = 0;
SDL_Joystick *gGameController = NULL;

// Callback for handling HTTP response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    struct json_object *parsed_json, *json_array;
    parsed_json = json_tokener_parse(ptr);
    if (!parsed_json) {
        return size * nmemb;
    }

    if (json_object_get_type(parsed_json) == json_type_array) {
        json_array = parsed_json;
        file_count = json_object_array_length(json_array);

        for (int i = 0; i < file_count && i < MAX_FILES; i++) {
            struct json_object *file_obj = json_object_array_get_idx(json_array, i);
            file_list[i] = strdup(json_object_get_string(file_obj));
        }
    }

    json_object_put(parsed_json);
    return size * nmemb;
}

// Fetch file list from server
void fetch_file_list() {
    CURL *curl;
    CURLcode res;
    const char *url = "https://m8slice.glitch.me/list-wav-files";

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

// Render text in SDL
void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, SDL_Color color, int highlight) {
    SDL_Color selectedColor = {191, 0, 255}; // Purple for selected item
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, highlight ? selectedColor : color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};

    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Download a selected file to /roms/ with animation
void download_file(const char *filename, SDL_Renderer *renderer, TTF_Font *font) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char url[512], output_path[256];

    snprintf(url, sizeof(url), "https://m8slice.glitch.me/%s", filename);
    snprintf(output_path, sizeof(output_path), DOWNLOAD_PATH "%s", filename);

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(output_path, "wb");
        if (!fp) {
            perror("Error opening file");
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // 🔥 Animated Progress Bar Characters
        char spinner[] = "-\\|/";
        int spinnerIndex = 0;
        int downloading = 1;
        int attempts = 0;

        // Show "Downloading..." message
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Color textColor = {200, 200, 200};

        while (downloading && attempts < 50) { // Max 10 seconds (50 * 200ms)
            attempts++;

            char downloadText[32];
            snprintf(downloadText, sizeof(downloadText), "Downloading... %c", spinner[spinnerIndex]);

            SDL_Surface *surface = TTF_RenderText_Solid(font, downloadText, textColor);
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 20, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_RenderPresent(renderer);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);

            spinnerIndex = (spinnerIndex + 1) % 4; // Cycle through spinner characters
            SDL_Delay(200); // Delay to create animation effect

            // Attempt download
            res = curl_easy_perform(curl);
            if (res == CURLE_OK) downloading = 0; // Exit loop on success
        }

        fclose(fp);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            // Show "BOOM! Download done." message
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_Surface *boomSurface = TTF_RenderText_Solid(font, "BOOM! Download done.", textColor);
            SDL_Texture *boomTexture = SDL_CreateTextureFromSurface(renderer, boomSurface);
            SDL_Rect boomRect = {SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 2 - 20, boomSurface->w, boomSurface->h};
            SDL_RenderCopy(renderer, boomTexture, NULL, &boomRect);
            SDL_RenderPresent(renderer);
            SDL_FreeSurface(boomSurface);
            SDL_DestroyTexture(boomTexture);

            printf("\n✅ BOOM! Download done: %s\n", filename);
            SDL_Delay(2000); // Show message for 2 seconds
        } else {
            printf("\n❌ Download failed: %s\n", filename);
        }
    }
}

// Main SDL2 UI function
void run_sdl_ui() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        printf("SDL Initialization failed: %s\n", SDL_GetError());
        return;
    }
    if (TTF_Init() < 0) {
        printf("SDL_ttf Initialization failed: %s\n", TTF_GetError());
        return;
    }

    SDL_Window *window = SDL_CreateWindow("M8 WAV Downloader", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Event event;
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);

    if (!font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        return;
    }

    if (SDL_NumJoysticks() > 0) {
        gGameController = SDL_JoystickOpen(0);
    }

    int running = 1;
    SDL_Color textColor = {200, 200, 200};

    while (running) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < file_count; i++) {
            render_text(renderer, font, file_list[i], 50, 50 + i * FONT_SIZE, textColor, i == selected_index);
        }

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_JOYBUTTONDOWN) {
                if (event.jbutton.button == 9 && selected_index < file_count - 1) selected_index++;
                if (event.jbutton.button == 8 && selected_index > 0) selected_index--;
                if (event.jbutton.button == 1) download_file(file_list[selected_index], renderer, font);
                if (event.jbutton.button == 12) while (SDL_WaitEventTimeout(&event, 500)) if (event.jbutton.button == 13) running = 0;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    fetch_file_list();
    if (file_count == 0) {
        printf("❌ No files available.\n");
        return 1;
    }
    run_sdl_ui();
    return 0;
}
