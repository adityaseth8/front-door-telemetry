#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
// #include <cstdint>

// #include <curl/curl.h>

struct SensorData {
    float x;
    float y;
    float z;
    char timestamp_us[64];
};

int build_json(float x, float y, float z, char *json, size_t json_size, char *timestamp) {
    return snprintf(json, json_size,
                    "{\"x\": %.3f, \"y\": %.3f, \"z\": %.3f, \"timestamp\": \"%s\"}",
                    x, y, z, timestamp);
}

void prepare_and_send_data(float x, float y, float z, char *timestamp) {

    // Build JSON string
    char json[128];
    build_json(x, y, z, json, sizeof(json), timestamp);
    printf("JSON: %s\n", json);



    
    // batch[batch_idx++] = (Sample){x, y, z, esp_timer_get_time()};
    // if (batch_idx >= BATCH_SIZE) {
    //     send_batch_http(batch, BATCH_SIZE);
    //     batch_idx = 0;
    // }
}

void format_timestamp(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(out, out_size, "%Y-%m-%d %H:%M:%S", t);
}

struct SensorData receive_sensor_data() {
    struct SensorData data;
    
    format_timestamp(data.timestamp_us, sizeof(data.timestamp_us)); // Placeholder - replace with actual timestamp
    data.x = (float)(rand() % 100) / 100;
    data.y = (float)(rand() % 100) / 100;
    data.z = (float)(rand() % 100) / 100;
    

    return data; // Placeholder - replace with actual sensor data
}

int main() {
    printf("Starting.\n");

    while (true) {
        struct SensorData d = receive_sensor_data();
        printf("Received - x: %f\ty: %f\tz: %f\n", d.x, d.y, d.z);
        prepare_and_send_data(d.x, d.y, d.z, d.timestamp_us);
        
        usleep(250000); // 25 ms

    }
    return 0;
}




// #define BATCH_SIZE 200  // 200 samples = ~10ms of data

// typedef struct { float x, y, z; uint64_t timestamp_us; } Sample;
// static Sample batch[BATCH_SIZE];
// static int batch_idx = 0;

// void on_new_sample(float x, float y, float z) {
//     batch[batch_idx++] = (Sample){x, y, z, esp_timer_get_time()};
//     if (batch_idx >= BATCH_SIZE) {
//         send_batch_http(batch, BATCH_SIZE);
//         batch_idx = 0;
//     }
// }


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