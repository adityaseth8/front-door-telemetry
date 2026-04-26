// needed to use mysys to install .dll files for curl.h - ucrt64 (what type is this)
// gcc write_to_fast_api.c -I"C:\msys64\ucrt64\include" -L"C:\msys64\ucrt64\lib" -lcurl -o your_program.exe

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <curl/curl.h>
#include "write_to_fast_api.h"
#include <string.h>

// #include <cstdint>

#define BATCH_SIZE 200
#define JSON_SIZE  160   // bumped: timestamp alone is 26 chars + float fields

static char batch[BATCH_SIZE][JSON_SIZE];
static int  batch_idx = 0;

char payload[BATCH_SIZE * JSON_SIZE + 32]; // extra for commas + brackets

// --- Timestamp: microsecond precision ---
void format_timestamp(char *out, size_t out_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);                                 // replaces time(NULL)
    time_t seconds = (time_t)tv.tv_sec;                      // convert to time_t for localtime
    struct tm *t = localtime(&seconds);
    int len = strftime(out, out_size, "%Y-%m-%d %H:%M:%S", t);
    
    // append microseconds
    snprintf(out + len, out_size - len, ".%06ld", tv.tv_usec);  // sprintf stores output to buffer
}

struct SensorData receive_sensor_data() {
    struct SensorData data;
    format_timestamp(data.timestamp_us, sizeof(data.timestamp_us));
    data.x = (float)(rand() % 100) / 100;
    data.y = (float)(rand() % 100) / 100;
    data.z = (float)(rand() % 100) / 100;
    return data;
}

// --- JSON builder: unchanged signature ---
int build_json(float x, float y, float z, char *json, size_t json_size, char *timestamp) {
    return snprintf(json, json_size,
                    "{\"x\": %.3f, \"y\": %.3f, \"z\": %.3f, \"timestamp\": \"%s\"}",
                    x, y, z, timestamp);
}

void add_to_batch(const char *json) {
    // add json entry to buffer containing batch data
    strncpy(batch[batch_idx], json, JSON_SIZE - 1);
    batch[batch_idx][JSON_SIZE - 1] = '\0';
    batch_idx++;
}

void send_batch_data(const char *json) {
    
    // Wrap JSON array in a list    
    payload[0] = '\0';
    strcat(payload, "[");

    for (int i = 0; i < BATCH_SIZE; i++) {
        strcat(payload, batch[i]);
        if (i < BATCH_SIZE - 1) {
            strcat(payload, ",");
        }
    }
    strcat(payload, "]");

    // setup curl and send payload
    CURL *curl = curl_easy_init();
    if (!curl) {
        printf("curl not found\n");
        return;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/data");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    exit(0);
}

int main() {
    printf("Starting...\n");
    while (true) {
        char json[JSON_SIZE];   // matched to macro (was hardcoded 128)

        struct SensorData d = receive_sensor_data();
        build_json(d.x, d.y, d.z, json, sizeof(json), d.timestamp_us);
        add_to_batch(json); 
        if (batch_idx >= BATCH_SIZE) {
            send_batch_data(json);
            usleep(250000); // 25 ms
        }
        

    }
    return 0;
}