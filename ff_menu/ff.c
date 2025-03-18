#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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

// Function prototypes
void stop_ffplay();
void launch_ffplay(const char *url);

const char *channels[MAX_CHANNELS] = {
"mjh-channel-9-vic", "mjh-gem-vic", "mjh-go-vic", "mjh-life-vic", "mjh-rush-vic", "mjh-seven-mel", "mjh-7two-mel", "mjh-7mate-mel", "mjh-7flix-mel", "mjh-abc-vic", "mjh-abc-kids", "mjh-abc-me", "mjh-abc-news", "mjh-sbs", "mjh-sbs-vic-hd", "mjh-sbs-viceland", "mjh-sbs-food", "mjh-sbs-world-movies", "mjh-sbs-nitv", "mjh-10-vic", "mjh-10peach-vic", "mjh-10bold-vic", "mjh-10shake-vic", "mjh-c31"
};

const char *urls[MAX_CHANNELS] = {
"https://i.mjh.nz/.r/channel-9-vic.m3u8", "https://i.mjh.nz/.r/gem-vic.m3u8", "https://i.mjh.nz/.r/go-vic.m3u8", "https://i.mjh.nz/.r/life-vic.m3u8", "https://i.mjh.nz/.r/rush-vic.m3u8", "https://i.mjh.nz/.r/seven-mel.m3u8", "https://i.mjh.nz/.r/7two-mel.m3u8", "https://i.mjh.nz/.r/7mate-mel.m3u8", "https://c.mjh.nz/abc-vic.m3u8", "https://c.mjh.nz/abc-kids.m3u8", "https://c.mjh.nz/abc-me.m3u8", "https://c.mjh.nz/abc-news.m3u8", "https://i.mjh.nz/.r/sbs.m3u8", "https://i.mjh.nz/.r/sbs-vic-hd.m3u8", "https://i.mjh.nz/.r/sbs-viceland.m3u8", "https://i.mjh.nz/.r/sbs-food.m3u8", "https://i.mjh.nz/.r/sbs-world-movies.m3u8", "https://i.mjh.nz/.r/sbs-nitv.m3u8", "https://i.mjh.nz/.r/10-vic.m3u8", "https://i.mjh.nz/.r/10peach-vic.m3u8", "https://i.mjh.nz/.r/10bold-vic.m3u8", "https://i.mjh.nz/.r/10shake-vic.m3u8", "https://i.mjh.nz/.r/c31.m3u8"
};

int selected_channel = 1;
pid_t ffplay_pid = -1;
int streaming = 0; // Flag to track if a stream is running

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void launch_ffplay(const char *url) {
    if (ffplay_pid > 0) {
        stop_ffplay(); // Ensure no previous ffplay is running
    }

    // Hide SDL window to prevent flickering
    SDL_HideWindow(window);

    ffplay_pid = fork();
    if (ffplay_pid == 0) {  // Child process
        execlp("ffplay", "ffplay", "-vcodec", "h264", "-vf", "scale=640:480", "-reconnect", "1", "-reconnect_streamed", "1", "-reconnect_delay_max", "5", url, NULL);

        exit(1); // If exec fails
    }

    streaming = 1; // Mark that a stream is running
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
    if (streaming) return; // Do not render while streaming

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Color green = {0, 255, 0};
    
    if (channels[selected_channel] != NULL) {
        SDL_Surface *surface = TTF_RenderText_Solid(font, channels[selected_channel], green);
        if (surface) {
            SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {50, SCREEN_HEIGHT / 2 - surface->h / 2, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }

    SDL_RenderPresent(renderer);
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
                if (event.jbutton.button == 0) {  // **B Button (Exit)**
                    stop_ffplay();
		   running = 0;
                }
                else if (!streaming && event.jbutton.button == 1) { // A to start stream
                    launch_ffplay(urls[selected_channel]);
                } 
                else if (streaming && event.jbutton.button == 2) { // X to stop stream
                    stop_ffplay();
		   
                } 
                else if (!streaming && event.jbutton.button == 8 && selected_channel > 0) { // UP
                    selected_channel--;
                } 
                else if (!streaming && event.jbutton.button == 9 && selected_channel < MAX_CHANNELS - 1) { // DOWN
                    selected_channel++;
                } 
                else if (!streaming && event.jbutton.button == 12 && event.jbutton.button == 13) { // Start + Select to quit
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
