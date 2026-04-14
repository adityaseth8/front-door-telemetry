/* ADXL345 Accelerometer Example (I2C Mode)

   This example reads X, Y, Z acceleration values from ADXL345 via I2C
   and outputs them via serial monitor in g units.
   
   Wiring (I2C):
   - ADXL345 VCC → ESP32 3.3V
   - ADXL345 GND → ESP32 GND
   - ADXL345 SDA → ESP32 GPIO 23
   - ADXL345 SCL → ESP32 GPIO 18
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "ADXL345_I2C";

// ADXL345 I2C address
#define ADXL345_ADDR 0x53

// ADXL345 Register addresses
#define ADXL345_DEVID 0x00
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0 0x32
#define ADXL345_DATAX1 0x33
#define ADXL345_DATAY0 0x34
#define ADXL345_DATAY1 0x35
#define ADXL345_DATAZ0 0x36
#define ADXL345_DATAZ1 0x37

// I2C pins
#define I2C_MASTER_SDA_IO 23
#define I2C_MASTER_SCL_IO 18
#define I2C_MASTER_FREQ_HZ 100000

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
    ESP_LOGI(TAG, "I2C initialized on SDA=GPIO23, SCL=GPIO18");
}

static uint8_t adxl345_read_register(uint8_t reg)
{
    uint8_t data;
    i2c_master_write_read_device(I2C_NUM_0, ADXL345_ADDR, 
                                 &reg, 1,
                                 &data, 1, 1000 / portTICK_PERIOD_MS);
    return data;
}

static void adxl345_write_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    i2c_master_write_to_device(I2C_NUM_0, ADXL345_ADDR,
                               data, 2, 1000 / portTICK_PERIOD_MS);
}

static void adxl345_init(void)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Wait for ADXL345 to power up
    
    // Check device ID
    uint8_t devid = adxl345_read_register(ADXL345_DEVID);
    ESP_LOGI(TAG, "ADXL345 Device ID: 0x%02X (should be 0xE5)", devid);

    // Set data format (16-bit mode, ±16g range)
    adxl345_write_register(ADXL345_DATA_FORMAT, 0x0B);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Enable measurement mode
    adxl345_write_register(ADXL345_POWER_CTL, 0x08);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    
    ESP_LOGI(TAG, "ADXL345 initialized");
}

static void read_adxl345(void)
{
    // Read X axis
    uint8_t x0 = adxl345_read_register(ADXL345_DATAX0);
    uint8_t x1 = adxl345_read_register(ADXL345_DATAX1);
    int16_t x = (int16_t)((x1 << 8) | x0);

    // Read Y axis
    uint8_t y0 = adxl345_read_register(ADXL345_DATAY0);
    uint8_t y1 = adxl345_read_register(ADXL345_DATAY1);
    int16_t y = (int16_t)((y1 << 8) | y0);

    // Read Z axis
    uint8_t z0 = adxl345_read_register(ADXL345_DATAZ0);
    uint8_t z1 = adxl345_read_register(ADXL345_DATAZ1);
    int16_t z = (int16_t)((z1 << 8) | z0);

    // Convert to g (±16g range, 13-bit data)
    float out_X = x * 0.0078f;  // 4mg per LSB for ±16g
    float out_Y = y * 0.0078f;
    float out_Z = z * 0.0078f;

    ESP_LOGI(TAG, "X: %.2fg   Y: %.2fg   Z: %.2fg", out_X, out_Y, out_Z);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ADXL345 example");

    i2c_init();
    adxl345_init();

    while (1) {
        read_adxl345();
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Read every 100ms
    }
}