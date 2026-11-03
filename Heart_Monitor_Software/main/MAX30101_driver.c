#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

//------------------------------- REGISTERS----------------------------------
//Status Register
#define Interrupt_Status_1    0x00
#define Interrupt_Status_2    0x01
#define Interrupt_Enable_1    0x02
#define Interrupt_Enable_2    0x03

// FIFO Registers
#define FIFO_Write_Pointer    0x04
#define Overflow_counter    0x05
#define FIFO_Read_Pointer    0x06
#define FIFO_Data_Pointer    0x07

// Configuration Registers
#define FIFO_Configuration    0x08
#define Mode    0x09
#define SpO2_Configuration    0x0A
#define Reserved1    0x0B
    //LED_Pulse_Amplitude
#define LED1_PA    0x0C
#define LED2_PA    0x0D
#define LED3_PA    0x0E
#define LED4_PA    0x0F

#define Multi_LED_Mode_Control_Registers1   0x11
#define Multi_LED_Mode_Control_Registers2   0x12
// Die Temperature registers

#define Die_Temp_Integer    0x1F
#define Die_Temp_Fraction    0x20
#define Die_Temperature_config    0x21
// Part ID registers

#define Revision_ID    0xFE
#define Part_ID        0x15
//------------------------------------------------------------------------------------------

//------------------Driver Specifics--------------------------------------------------------
#define I2C_MASTER_SCL_IO 5
#define I2C_MASTER_SDA_IO 6
#define SCL_SPEED 100 * 1000
#define MAX30101_DATA_LENGTH_RD 9 // 9bytes of data. change  later
#define MAX30101_DATA_LENGTH_WR 1 // 1byte of data



i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_PORT_NUM_0,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    //.flags.enable_internal_pullup =
};

i2c_master_bus_handle_t bus_handle;

ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

i2c_device_config_t MAX30101_cfg = {
    .dev_addr_length =  I2C_ADDR_BIT_LEN_7,
    .device_address = 0x57,
    .scl_speed_hz = SCL_SPEED,
};

i2c_master_dev_handle_t MAX30101_handle;
ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &MAX30101_cfg, &MAX30101_handle));

uint8_t data_rd[MAX30101_DATA_LENGTH_RD];
uint8_t data_wr[MAX30101_DATA_LENGTH_WR];

ESP_ERROR_CHECK(i2c_master_transmit(MAX30101_handle, data_wr, MAX30101_DATA_LENGTH_WR, -1)); //write to slave
ESP_ERROR_CHECK(i2c_master_receive(MAX30101_handle, data_rd, MAX30101_DATA_LENGTH_RD, -1)); //read from slave
 

\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
#ifndef MAX30101_H
#define MAX30101_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX30101_I2C_ADDR           0x57

// Core Register Map
#define MAX30101_REG_INTR_STATUS_1  0x00
#define MAX30101_REG_INTR_STATUS_2  0x01
#define MAX30101_REG_INTR_ENABLE_1  0x02
#define MAX30101_REG_INTR_ENABLE_2  0x03
#define MAX30101_REG_FIFO_WR_PTR    0x04
#define MAX30101_REG_OVF_COUNTER    0x05
#define MAX30101_REG_FIFO_RD_PTR    0x06
#define MAX30101_REG_FIFO_DATA      0x07
#define MAX30101_REG_FIFO_CONFIG    0x08
#define MAX30101_REG_MODE_CONFIG    0x09
#define MAX30101_REG_SPO2_CONFIG    0x0A
#define MAX30101_REG_LED1_PA        0x0C
#define MAX30101_REG_LED2_PA        0x0D
#define MAX30101_REG_LED3_PA        0x0E
#define MAX30101_REG_PILOT_PA       0x10
#define MAX30101_REG_MULTI_LED_1    0x11
#define MAX30101_REG_MULTI_LED_2    0x12
#define MAX30101_REG_TEMP_INTR      0x1F
#define MAX30101_REG_TEMP_FRAC      0x20
#define MAX30101_REG_PART_ID        0xFF

// Configuration Constants
#define MAX30101_EXPECTED_PART_ID   0x15

typedef struct {
    uint32_t red;
    uint32_t ir;
} max30101_sample_t;

typedef struct {
    i2c_master_dev_handle_t i2c_dev;
} max30101_dev_t;

/**
 * @brief Initialize and configure the MAX30101 sensor.
 * 
 * @param bus_handle Initialized I2C master bus handle.
 * @param dev_out Pointer to store the created MAX30101 device handle wrapper.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t max30101_init(i2c_master_bus_handle_t bus_handle, max30101_dev_t *dev_out);

/**
 * @brief Reset the MAX30101 device.
 * 
 * @param dev MAX30101 device handle wrapper.
 * @return esp_err_t 
 */
esp_err_t max30101_reset(max30101_dev_t *dev);

/**
 * @brief Read available data samples from the FIFO buffer.
 * 
 * @param dev MAX30101 device handle wrapper.
 * @param samples Pointer to buffer to hold parsed samples.
 * @param max_samples Maximum number of samples to read.
 * @param num_read Pointer to store actual number of samples read.
 * @return esp_err_t 
 */
esp_err_t max30101_read_fifo(max30101_dev_t *dev, max30101_sample_t *samples, uint8_t max_samples, uint8_t *num_read);

#ifdef __cplusplus
}
#endif

#endif // MAX30101_H
////////////////////////////////////////////////////////////////
#include "max30101.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30101";

static esp_err_t max30101_write_reg(max30101_dev_t *dev, uint8_t reg, uint8_t val) {
    uint8_t write_buf[2] = {reg, val};
    return i2c_master_transmit(dev->i2c_dev, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));
}

static esp_err_t max30101_read_reg(max30101_dev_t *dev, uint8_t reg, uint8_t *val) {
    return i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, val, 1, pdMS_TO_TICKS(100));
}

esp_err_t max30101_reset(max30101_dev_t *dev) {
    // Set Reset bit in Mode Configuration Register
    esp_err_t ret = max30101_write_reg(dev, MAX30101_REG_MODE_CONFIG, 0x40);
    if (ret != ESP_OK) return ret;
    
    // Wait for reset to complete (poll reset bit or delay)
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t max30101_init(i2c_master_bus_handle_t bus_handle, max30101_dev_t *dev_out) {
    if (!bus_handle || !dev_out) {
        return ESP_ERR_INVALID_ARG;
    }

    // Configure I2C device parameters for ESP-IDF v5 master API
    i2c_device_config_t i2c_dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX30101_I2C_ADDR,
        .scl_speed_hz = 400000, // 400 kHz Fast Mode
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &dev_out->i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add MAX30101 to I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Verify Part ID
    uint8_t part_id = 0;
    ret = max30101_read_reg(dev_out, MAX30101_REG_PART_ID, &part_id);
    if (ret != ESP_OK || part_id != MAX30101_EXPECTED_PART_ID) {
        ESP_LOGE(TAG, "Invalid Part ID: 0x%02X (Expected: 0x%02X)", part_id, MAX30101_EXPECTED_PART_ID);
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(TAG, "MAX30101  detected successfully (Part ID: 0x%02X)", part_id);

    // 1. Reset device
    ret = max30101_reset(dev_out);
    if (ret != ESP_OK) return ret;

    // 2. Configure FIFO Settings (Sample Averaging = 4, FIFO Rollover Enabled, FIFO Almost Full = 17 samples)
    ret = max30101_write_reg(dev_out, MAX30101_REG_FIFO_CONFIG, (0x02 << 5) | (1 << 4) | 0x0F);
    if (ret != ESP_OK) return ret;

    // 3. Set Mode: SpO2 Mode (Red + IR LEDs enabled -> Mode = 0x03)
    ret = max30101_write_reg(dev_out, MAX30101_REG_MODE_CONFIG, 0x03);
    if (ret != ESP_OK) return ret;

    // 4. Set SpO2 Configuration (ADC Range = 4096nA, Sample Rate = 100Hz, LED Pulse Width = 411µs / 18-bit res)
    ret = max30101_write_reg(dev_out, MAX30101_REG_SPO2_CONFIG, (0x01 << 5) | (0x01 << 2) | 0x03);
    if (ret != ESP_OK) return ret;

    // 5. Set LED Pulse Amplitudes (~6.4mA for Red and IR)
    ret = max30101_write_reg(dev_out, MAX30101_REG_LED1_PA, 0x24); // Red LED
    if (ret != ESP_OK) return ret;
    ret = max30101_write_reg(dev_out, MAX30101_REG_LED2_PA, 0x24); // IR LED
    if (ret != ESP_OK) return ret;

    // Clear FIFO pointers
    max30101_write_reg(dev_out, MAX30101_REG_FIFO_WR_PTR, 0x00);
    max30101_write_reg(dev_out, MAX30101_REG_OVF_COUNTER, 0x00);
    max30101_write_reg(dev_out, MAX30101_REG_FIFO_RD_PTR, 0x00);

    ESP_LOGI(TAG, "MAX30101 initialized and configured for SpO2 mode.");
    return ESP_OK;
}

esp_err_t max30101_read_fifo(max30101_dev_t *dev, max30101_sample_t *samples, uint8_t max_samples, uint8_t *num_read) {
    uint8_t wr_ptr = 0, rd_ptr = 0;
    
    if (max30101_read_reg(dev, MAX30101_REG_FIFO_WR_PTR, &wr_ptr) != ESP_OK) return ESP_FAIL;
    if (max30101_read_reg(dev, MAX30101_REG_FIFO_RD_PTR, &rd_ptr) != ESP_OK) return ESP_FAIL;

    int samples_to_read = (wr_ptr - rd_ptr) & 0x1F;
    if (samples_to_read == 0) {
        *num_read = 0;
        return ESP_OK;
    }

    if (samples_to_read > max_samples) {
        samples_to_read = max_samples;
    }

    // Each sample in SpO2 mode (Red + IR) consists of 6 bytes (3 bytes per channel)
    uint8_t bytes_to_read = samples_to_read * 6;
    uint8_t read_buffer[96]; // Max typical batch chunk
    if (bytes_to_read  > sizeof(read_buffer)) {
        bytes_to_read = sizeof(read_buffer);
        samples_to_read = bytes_to_read / 6;
    }

    uint8_t reg = MAX30101_REG_FIFO_DATA;
    esp_err_t ret = i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, read_buffer, bytes_to_read, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    for (int i = 0; i < samples_to_read; i++) {
        int idx = i * 6;
        // Parse 18-bit Red channel data
        samples[i].red = ((uint32_t)read_buffer[idx + 0] << 16) |
                           ((uint32_t)read_buffer[idx + 1] << 8)  |
                           (uint32_t)read_buffer[idx + 2];
        samples[i].red &= 0x03FFFF; // Mask to 18 bits

        // Parse 18-bit IR channel data
        samples[i].ir =  ((uint32_t)read_buffer[idx + 3] << 16) |
                           ((uint32_t)read_buffer[idx + 4] << 8)  |
                           (uint32_t)read_buffer[idx + 5];
        samples[i].ir &= 0x03FFFF;  // Mask to 18 bits
    }

    *num_read = samples_to_read;
    return ESP_OK;
}