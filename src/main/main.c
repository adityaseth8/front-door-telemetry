#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "LIS3DH_I2C";

// I2C address (SDO → GND)
#define LIS3DH_ADDR 0x18

// LIS3DH registers
#define LIS3DH_WHO_AM_I   0x0F
#define LIS3DH_CTRL_REG1  0x20

#define LIS3DH_OUT_X_L    0x28
#define LIS3DH_OUT_X_H    0x29
#define LIS3DH_OUT_Y_L    0x2A
#define LIS3DH_OUT_Y_H    0x2B
#define LIS3DH_OUT_Z_L    0x2C
#define LIS3DH_OUT_Z_H    0x2D

// I2C pins
#define I2C_MASTER_SDA_IO 23
#define I2C_MASTER_SCL_IO 18
#define I2C_MASTER_FREQ_HZ 100000

// ---------------- I2C INIT ----------------
static void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    ESP_LOGI(TAG, "I2C initialized");
}

// ---------------- LOW LEVEL ----------------
static uint8_t lis3dh_read_register(uint8_t reg)
{
    uint8_t data;
    i2c_master_write_read_device(I2C_NUM_0, LIS3DH_ADDR,
                                 &reg, 1,
                                 &data, 1,
                                 1000 / portTICK_PERIOD_MS);
    return data;
}

static void lis3dh_write_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    i2c_master_write_to_device(I2C_NUM_0, LIS3DH_ADDR,
                               data, 2,
                               1000 / portTICK_PERIOD_MS);
}

// ---------------- INIT ----------------
static void lis3dh_init(void)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);

    uint8_t id = lis3dh_read_register(LIS3DH_WHO_AM_I);
    ESP_LOGI(TAG, "WHO_AM_I: 0x%02X (expected 0x33)", id);

    // Enable sensor: 100Hz, all axes
    lis3dh_write_register(LIS3DH_CTRL_REG1, 0x57);

    vTaskDelay(50 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "LIS3DH initialized");
}

// ---------------- READ DATA ----------------
static void read_lis3dh(void)
{
    uint8_t reg = LIS3DH_OUT_X_L | 0x80; // auto-increment
    uint8_t data[6];

    i2c_master_write_read_device(I2C_NUM_0, LIS3DH_ADDR,
                                 &reg, 1,
                                 data, 6,
                                 1000 / portTICK_PERIOD_MS);

    int16_t x = (int16_t)((data[1] << 8) | data[0]);
    int16_t y = (int16_t)((data[3] << 8) | data[2]);
    int16_t z = (int16_t)((data[5] << 8) | data[4]);

    // Scale for ±2g (default)
    float ax = x * 0.000061f;
    float ay = y * 0.000061f;
    float az = z * 0.000061f;

    ESP_LOGI(TAG, "X: %.3fg  Y: %.3fg  Z: %.3fg", ax, ay, az);
}

// ---------------- MAIN ----------------
void app_main(void)
{
    ESP_LOGI(TAG, "Starting LIS3DH example");

    i2c_init();
    lis3dh_init();

    while (1) {
        read_lis3dh();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}