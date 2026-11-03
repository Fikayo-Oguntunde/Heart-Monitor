#ifndef MAX30101_DRIVER_H
#define MAX30101_DRIVER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

//------------------------------- REGISTERS----------------------------------
//Status Register
#define INTERRUPT_STATUS_1    0x00
#define INTERRUPT_STATUS_2    0x01
#define INTERRUPT_ENABLE_1    0x02
#define INTERRUPT_ENABLE_2    0x03

// FIFO Registers
#define FIFO_WRITE_POINTER    0x04
#define OVERFLOW_COUNTER    0x05
#define FIFO_READ_POINTER    0x06
#define FIFO_DATA_POINTER    0x07

// Configuration Registers
#define FIFO_CONFIGURATION    0x08
#define MODE_CONFIGURATION    0x09
#define SPO2_CONFIGURATION    0x0A
#define RESERVED1    0x0B
    //LED_Pulse_Amplitude
#define LED1_PA    0x0C
#define LED2_PA    0x0D
#define LED3_PA    0x0E
#define LED4_PA    0x0F

#define MULTI_LED_MODE_CONTROL_REGISTERS1   0x11
#define MULTI_LED_MODE_CONTROL_REGISTERS2   0x12
// Die Temperature registers

#define DIE_TEMP_INTEGER    0x1F
#define DIE_TEMP_FRACTION    0x20
#define DIE_TEMPERATURE_CONFIG    0x21
// Part ID registers

#define REVISION_ID    0xFE
#define PART_ID        0xFF

//PERSONAL CONFIG
#define LED_CURR_CONTROL 0xFF // this has been set randomly make sure to set it to something better later on
#define FIFO_HR_MODE_CONFIG 0xFF // this has been set randomly make sure to set it to something better later on
#define FIFO_SPO2_MODE_CONFIG 0xFF // this has been set randomly make sure to set it to something better later on
#define FIFO_MULTI_LED_MODE_CONFIG 0xFF // this has been set randomly make sure to set it to something better later on

#define SP02_MODE_CONFIG 0xFF // this has been set randomly make sure to set it to something better later on
#define MULTI_LED_MODE_CONFIG_1 0xFF // this has been set randomly make sure to set it to something better later on
#define MULTI_LED_MODE_CONFIG_2 0xFF // this has been set randomly make sure to set it to something better later on

#define FIFO_BUFFER_SIZE  0xFF // this has been set randomly make sure to set it to something better later on

typedef enum{
    HR_MODE = 0x02,
    SPO2_MODE  = 0x03,
    MULTI_LED_MODE = 0x07
}mode_t;

extern mode_t MODE_STATE; 

static esp_err_t read(i2c_master_dev_handle_t i2c_dev, uint8_t addr, uint8_t *read_val);
static esp_err_t max_write(i2c_master_dev_handle_t i2c_dev, uint8_t addr, uint8_t val);
static esp_err_t initialize(i2c_master_dev_handle_t i2c_dev);
static esp_err_t intr_config(i2c_master_dev_handle_t i2c_dev);
static esp_err_t FIFO_config_init(i2c_master_dev_handle_t i2c_dev);
static esp_err_t LED_curr_contr(i2c_master_dev_handle_t i2c_dev, uint8_t led_1, uint8_t led_2, uint8_t led_3, uint8_t led_4);
static esp_err_t set_mode(i2c_master_dev_handle_t i2c_dev, mode_t mode);
esp_err_t MAX30101_initialize(void);

#endif


