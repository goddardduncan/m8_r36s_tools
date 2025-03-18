#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ncurses.h>
#include <json-c/json.h>

#define MAX_FILES 100

char *file_list[MAX_FILES]; // Array to hold file names
int file_count = 0;
int selected_index = 0;

// Callback function to capture response from the server
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

// Function to fetch the list of files
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

// Function to download the selected file
void download_file(const char *filename) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char url[512], output_path[256];

    snprintf(url, sizeof(url), "https://m8slice.glitch.me/%s", filename);
    snprintf(output_path, sizeof(output_path), "%s", filename);

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
        res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            printf("\n✅ Download complete: %s\n", filename);
        } else {
            printf("\n❌ Download failed: %s\n", filename);
        }
    }
}

// Function to display the menu using ncurses
void display_menu() {
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    while (1) {
        clear();
        mvprintw(0, 0, "Use UP/DOWN to select a file. Press ENTER to download. Press 'q' to quit.");

        for (int i = 0; i < file_count; i++) {
            if (i == selected_index) {
                attron(A_REVERSE);
                mvprintw(i + 2, 2, "%s", file_list[i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(i + 2, 2, "%s", file_list[i]);
            }
        }

        int ch = getch();
        if (ch == KEY_UP && selected_index > 0) {
            selected_index--;
        } else if (ch == KEY_DOWN && selected_index < file_count - 1) {
            selected_index++;
        } else if (ch == '\n') {
            break;
        } else if (ch == 'q') {
            endwin();
            exit(0);
        }
    }

    endwin();
}

// Main function
int main() {
    fetch_file_list();
    if (file_count == 0) {
        printf("❌ No files available.\n");
        return 1;
    }

    display_menu();

    printf("\nDownloading: %s\n", file_list[selected_index]);
    download_file(file_list[selected_index]);

    return 0;
}
