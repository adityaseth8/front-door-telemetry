#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "esp_http_client.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "esp_sntp.h" // clock

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD
#define FASTAPI_URL CONFIG_FASTAPI_URL

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static const char *TAG = "LIS3DH_SPI";

// SPI pins
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// LIS3DH registers
#define LIS3DH_WHO_AM_I   0x0F
#define LIS3DH_CTRL_REG1  0x20
#define LIS3DH_STATUS_REG 0x27

#define LIS3DH_OUT_X_L    0x28

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    else if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        esp_wifi_connect();  // auto reconnect
}

void wifi_init() {
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // block until connected
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected");
}

static spi_device_handle_t spi;

void sync_time() {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    while (now < 100000) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        time(&now);
    }
    ESP_LOGI(TAG, "Time synced");
}


#define BATCH_SIZE 50
#define JSON_SIZE  160   // bumped: timestamp alone is 26 chars + float fields

static char batch[BATCH_SIZE][JSON_SIZE];
static int  batch_idx = 0;

struct SensorData {
	float x, y, z;
	char  timestamp_us[64];
};

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

void format_timestamp(char *out, size_t out_size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t seconds = (time_t)tv.tv_sec;

    struct tm t;
    gmtime_r(&seconds, &t);
    
    int len = strftime(out, out_size, "%Y-%m-%dT%H:%M:%S", &t);
    snprintf(out + len, out_size - len, ".%06ldZ", tv.tv_usec);
    // int len = strftime(out, out_size, "%Y-%m-%d %H:%M:%S", t);
    
    // append microseconds
    // snprintf(out + len, out_size - len, ".%06ld", tv.tv_usec);  // sprintf stores output to buffer
}


void send_batch_data(const char *payload) {
    esp_http_client_config_t config = {
        .url = FASTAPI_URL, // IPv4 address of FastAPI server 
        .method = HTTP_METHOD_POST,
        .timeout_ms = 3000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

// ---------------- SPI WRITE ----------------
static void lis3dh_write(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg & 0x7F, value};

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx,
    };

    spi_device_transmit(spi, &t);
}

// ---------------- SPI READ ----------------
static void lis3dh_read_multi(uint8_t reg, uint8_t *data, int len)
{
    uint8_t tx[len + 1];
    uint8_t rx[len + 1];

    memset(tx, 0, sizeof(tx));
    memset(rx, 0, sizeof(rx));

    tx[0] = reg | 0x80 | 0x40; // read + auto-increment

    spi_transaction_t t = {
        .length = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx
    };

    spi_device_transmit(spi, &t);

    memcpy(data, &rx[1], len);
}

// ---------------- INIT SENSOR ----------------
static void lis3dh_init()
{
    vTaskDelay(100 / portTICK_PERIOD_MS);

    uint8_t who = 0;
    lis3dh_read_multi(LIS3DH_WHO_AM_I, &who, 1);
    ESP_LOGI(TAG, "WHO_AM_I: 0x%02X (expected 0x33)", who);

    // 100Hz, all axes enabled
    lis3dh_write(LIS3DH_CTRL_REG1, 0x57);

    ESP_LOGI(TAG, "LIS3DH initialized");
}

// ---------------- READ ACCEL ----------------

static char payload[BATCH_SIZE * JSON_SIZE + 32]; // extra for commas + brackets

static void lis3dh_read_and_batch()
{
    struct SensorData data;
    format_timestamp(data.timestamp_us, sizeof(data.timestamp_us));
    uint8_t raw[6];
    lis3dh_read_multi(LIS3DH_OUT_X_L, raw, 6);

    int16_t x = (int16_t)(raw[1] << 8 | raw[0]);
    int16_t y = (int16_t)(raw[3] << 8 | raw[2]);
    int16_t z = (int16_t)(raw[5] << 8 | raw[4]);

    data.x = x * 0.000061f;
    data.y = y * 0.000061f;
    data.z = z * 0.000061f;

    ESP_LOGI(TAG, "X: %.3fg Y: %.3fg Z: %.3fg", data.x, data.y, data.z);

    char json[JSON_SIZE];
    build_json(data.x, data.y, data.z, json, sizeof(json), data.timestamp_us);
    add_to_batch(json);

    if (batch_idx >= BATCH_SIZE) {
        // Send batch to FastAPI server
        ESP_LOGI(TAG, "Batch full, sending data to FastAPI server...");
        payload[0] = '\0';
        strcat(payload, "[");
        for (int i = 0; i < BATCH_SIZE; i++) {
            strcat(payload, batch[i]);
            if (i < BATCH_SIZE - 1) {
                strcat(payload, ",");
            }
        }
        strcat(payload, "]");
        send_batch_data(payload);
        batch_idx = 0; // reset batch
    }
}

// ---------------- SPI INIT ----------------
static void spi_init()
{
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 500 * 1000,   // slower = more stable
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1
    };

    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
}

// ---------------- MAIN ----------------
void app_main(void)
{
    ESP_LOGI(TAG, "Starting LIS3DH SPI example");

    wifi_init();  // must be first

    sync_time(); // using NTP


    spi_init();
    lis3dh_init();

    while (1)
    {
        uint8_t status = 0;
        // check DATA READY bit
        lis3dh_read_multi(LIS3DH_STATUS_REG, &status, 1);
        if (status & 0x08) {
            lis3dh_read_and_batch();
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}