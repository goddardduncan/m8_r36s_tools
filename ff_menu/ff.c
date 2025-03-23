#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <json-c/json.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define FONT_SIZE 24
#define MAX_CHANNELS 24
#define CHANNELS_FILE "/roms/tvradio/channelsTV.json"

// Function prototypes
void stop_ffplay();
void launch_ffplay(const char *url, TTF_Font *font);  // Fix: Add TTF_Font *font as parameter
void render(SDL_Renderer *renderer, TTF_Font *font);  // Declare render() at the top


const char *channels[MAX_CHANNELS];
const char *urls[MAX_CHANNELS];
int channel_count = 0;

int selected_channel = 1;
pid_t ffplay_pid = -1;
int streaming = 0; // Flag to track if a stream is running

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void launch_ffplay(const char *url, TTF_Font *font) {  // Fix: Add font parameter
    if (ffplay_pid > 0) {
        stop_ffplay(); // Ensure no previous ffplay is running
    }

    streaming = 1; // Set streaming flag
    

    // Force multiple renders for 5 seconds
    Uint32 start_time = SDL_GetTicks();
    while (SDL_GetTicks() - start_time < 5000) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {}  // Process SDL events

        render(renderer, font);  // Fix: Now properly passes font to render()
        SDL_RenderPresent(renderer); // Force SDL to update screen
        
    }

    

    SDL_HideWindow(window); // Hide the window AFTER displaying blue text

    ffplay_pid = fork();
    if (ffplay_pid == 0) {  // Child process
        execlp("ffplay", "ffplay", "-vcodec", "h264", "-vf", "scale=640:380", 
               "-reconnect", "1", "-reconnect_streamed", "1", "-reconnect_delay_max", 
               "5", url, NULL);
        exit(1); // If exec fails
    }
}


void stop_ffplay() {
    if (ffplay_pid > 0) {
        printf("Stopping ffplay (PID: %d)\n", ffplay_pid);
        fflush(stdout); // Ensure output is printed immediately
        
        kill(ffplay_pid, SIGTERM); // Try SIGTERM first
        sleep(1); // Give ffplay a moment to close
        
        if (waitpid(ffplay_pid, NULL, WNOHANG) == 0) {
            printf("ffplay did not exit, sending SIGKILL\n");
            kill(ffplay_pid, SIGKILL); // Force kill if still running
        }
        
        waitpid(ffplay_pid, NULL, 0); // Ensure process cleanup
        ffplay_pid = -1;
        printf("ffplay stopped\n");
    }

    streaming = 0; // Reset streaming flag
    sleep(5);
    SDL_ShowWindow(window);
}


void render(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255};
    SDL_Color purple = {128, 0, 128};  // Default for selection
    SDL_Color blue = {0, 0, 255};      // Should be blue while streaming

    int start_index = selected_channel - 4;
    if (start_index < 0) start_index = 0;
    int end_index = start_index + 9;
    if (end_index >= MAX_CHANNELS) end_index = MAX_CHANNELS - 1;

    for (int i = start_index; i <= end_index; i++) {
        SDL_Color color = white;

        if (i == selected_channel) {
            if (streaming) {
                color = blue;  // Set to blue when streaming
                
            } else {
                color = purple;  // Otherwise, should be purple
                
            }
        }

        SDL_Surface *surface = TTF_RenderText_Solid(font, channels[i], color);
        if (!surface) {
            
            continue;
        }
        
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = {50, (SCREEN_HEIGHT / 2) + (i - selected_channel) * 30, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

int load_channels(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Failed to open %s\n", filename);
        return 0;
    }

    struct json_object *parsed_json;
    struct json_object *obj;

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);

    char *json_data = malloc(fsize + 1);
    fread(json_data, 1, fsize, fp);
    json_data[fsize] = 0;
    fclose(fp);

    parsed_json = json_tokener_parse(json_data);
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

    window = SDL_CreateWindow("Channel Selector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", FONT_SIZE);

    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    int running = 1;
    SDL_Event event;

    while (running) {
    render(renderer, font);

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) running = 0;

        if (event.type == SDL_JOYBUTTONDOWN) {
            if (event.jbutton.button == 2) {  // X Button (Exit)
                stop_ffplay();
                running = 0;
            } 
            else if (!streaming && event.jbutton.button == 1) { // A to start stream
                launch_ffplay(urls[selected_channel], font);  // Fix: Pass font to launch_ffplay()
            } 
            else if (streaming && event.jbutton.button == 0) { // B to stop stream
                stop_ffplay();
            } 
            else if (!streaming && event.jbutton.button == 8 && selected_channel > 0) { // UP
                selected_channel--;
            } 
            else if (!streaming && event.jbutton.button == 9 && selected_channel < MAX_CHANNELS - 1) { // DOWN
                selected_channel++;
            } 
            else if (!streaming && event.jbutton.button == 12 && event.jbutton.button == 13) { // Start + Select to quit
                stop_ffplay();
                running = 0;
            }
        }
    }
}


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
	exit(0);

}


int main() {
    if (!load_channels(CHANNELS_FILE)) {
    	printf("Could not load channelsTV.json\n");
    	return 1;
    }
    run_sdl_ui();
    return 0;
}
