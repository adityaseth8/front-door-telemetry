#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

// #include <cstdint>

// #include <curl/curl.h>

#define BATCH_SIZE 200
#define JSON_SIZE  160   // bumped: timestamp alone is 26 chars + float fields

static char batch[BATCH_SIZE][JSON_SIZE];
static int  batch_idx = 0;

struct SensorData {
    float x, y, z;
    char  timestamp_us[64];
};

// --- Timestamp: microsecond precision ---
void format_timestamp(char *out, size_t out_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);                         // replaces time(NULL)
    struct tm *t = localtime((const time_t*)&tv.tv_sec);
    int len = strftime(out, out_size, "%Y-%m-%d %H:%M:%S", t);
    snprintf(out + len, out_size - len, ".%06ld", tv.tv_usec);  // append microseconds
}

// --- JSON builder: unchanged signature ---
int build_json(float x, float y, float z, char *json, size_t json_size, char *timestamp) {
    return snprintf(json, json_size,
                    "{\"x\": %.3f, \"y\": %.3f, \"z\": %.3f, \"timestamp\": \"%s\"}",
                    x, y, z, timestamp);
}

// --- Batch flush: wraps in a JSON array for HTTP ---
void flush_batch() {
    // TODO: replace printf block with esp_http_client POST
    // Body should be:  [sample0, sample1, ..., sample199]
    printf("!!!Sending batch:\n[\n");
    for (int i = 0; i < BATCH_SIZE; i++) {
        printf("  %s%s\n", batch[i], i < BATCH_SIZE - 1 ? "," : "");
    }
    printf("]\n\n");
    batch_idx = 0;
}

// --- send_data: removed redundant stack copy ---
void send_data(const char *json) {
    strncpy(batch[batch_idx], json, JSON_SIZE - 1);
    batch[batch_idx][JSON_SIZE - 1] = '\0';
    batch_idx++;

    if (batch_idx >= BATCH_SIZE) {
        flush_batch();    // extracted so the HTTP stub lives in one place
    }
}

// --- prepare_and_send_data: unchanged ---
void prepare_and_send_data(float x, float y, float z, char *timestamp) {
    char json[JSON_SIZE];   // matched to macro (was hardcoded 128)
    build_json(x, y, z, json, sizeof(json), timestamp);
    send_data(json);
}

// --- receive_sensor_data: unchanged except timestamp call is same signature ---
struct SensorData receive_sensor_data() {
    struct SensorData data;
    format_timestamp(data.timestamp_us, sizeof(data.timestamp_us));
    data.x = (float)(rand() % 100) / 100;
    data.y = (float)(rand() % 100) / 100;
    data.z = (float)(rand() % 100) / 100;
    return data;
}

int main() {
    printf("Starting.\n");
    while (true) {
        struct SensorData d = receive_sensor_data();
        prepare_and_send_data(d.x, d.y, d.z, d.timestamp_us);
    }
    return 0;
}
// int main() {
    
//     printf("Starting sensor stream...\n");
//     CURL *curl = curl_easy_init();

//     if (!curl) {
//         fprintf(stderr, "Failed to init curl\n");
//         return 1;
//     }

//     // Set constant options ONCE
//     curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8000/data");

//     struct curl_slist *headers = NULL;
//     headers = curl_slist_append(headers, "Content-Type: application/json");
//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

//     while (true) {
//         float x = (float)(rand() % 100) / 100;
//         float y = (float)(rand() % 100) / 100;
//         float z = (float)(rand() % 100) / 100;

//         printf("x: %f\ty: %f\tz: %f\n", x, y, z);

//         // Build JSON string
//         char json[128];
//         snprintf(json, sizeof(json),
//                  "{\"x\": %.3f, \"y\": %.3f, \"z\": %.3f}",
//                  x, y, z);

//         // Attach JSON payload
//         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

//         // Send request
//         CURLcode res = curl_easy_perform(curl);

//         if (res != CURLE_OK) {
//             fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
//         }

//         usleep(250000); // 25 ms
//     }

//     curl_easy_cleanup(curl);
//     return 0;
// }