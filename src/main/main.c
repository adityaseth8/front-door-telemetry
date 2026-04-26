#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

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

static spi_device_handle_t spi;

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
static void lis3dh_read()
{
    uint8_t data[6];

    lis3dh_read_multi(LIS3DH_OUT_X_L, data, 6);

    int16_t x = (int16_t)(data[1] << 8 | data[0]);
    int16_t y = (int16_t)(data[3] << 8 | data[2]);
    int16_t z = (int16_t)(data[5] << 8 | data[4]);

    float ax = x * 0.000061f;
    float ay = y * 0.000061f;
    float az = z * 0.000061f;

    ESP_LOGI(TAG, "X: %.3fg Y: %.3fg Z: %.3fg", ax, ay, az);
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

    spi_init();
    lis3dh_init();

    while (1)
    {
        uint8_t status = 0;

        // check DATA READY bit
        lis3dh_read_multi(LIS3DH_STATUS_REG, &status, 1);

        if (status & 0x08) {
            lis3dh_read();
        }

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}