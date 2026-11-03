#include <tester_driver.h>

#define I2C_CLK_SRC //remember to set a value here
#define I2C_MASTER_SCL_IO
#define I2C_MASTER_SDA_IO
#define SCL_SPEED 

#define INTR_STATUS_1_STATE 0xE0 //enable all interrupts
#define INTR_STATUS_2_STATE 0x01 //enable all interrupts
#define FIFO_CONFIG_STATE 0x60

#define FIFO_POR_STATE 0x00


static const char *TAG = "MAX30101";

//read and write!
static esp_err_t read(i2c_master_dev_handle_t i2c_dev, uint8_t addr, uint8_t *read_val){
    return i2c_master_transmit_receive(i2c_dev, &addr, 1, read_val, 1, -1);
}
static esp_err_t write(i2c_master_dev_handle_t i2c_dev, uint8_t addr, uint8_t val){
    uint8_t write_buf[2]={addr, val};
    return i2c_master_transmit(i2c_dev, write_buf, 2, -1);
}


static esp_err_t ID_initialize(i2c_master_dev_handle_t i2c_dev){
    uint8_t part_id = 0;
    if(read(i2c_dev, 0xFF, &part_id) == ESP_OK){
        if (part_id == PART_ID){
            return ESP_OK;
        }
        else{
            ESP_LOGE(TAG, "Failed to Identify ID");
            return ESP_FAIL;
        }
    }else{
        ESP_LOGE(TAG, "Failed to read ID");
        return ESP_FAIL;
    }     
}


static esp_err_t intr_config(i2c_master_dev_handle_t i2c_dev){
    
    esp_err_t ret_status_1 = write(i2c_dev, INTERRUPT_ENABLE_1, INTR_STATUS_1_STATE);
    esp_err_t ret_status_2 = write(i2c_dev, INTERRUPT_ENABLE_2, INTR_STATUS_2_STATE);

    if(ret_status_1 == ESP_OK && ret_status_2 == ESP_OK ){
        return ESP_OK;
    }else if(ret_status_1 == ESP_OK && ret_status_2 =! ESP_OK){
        // writing INTR_STATUS_2 failed
        ESP_LOGE(TAG, ":INTR_STATUS_2 write failed");
        return ret_status_2;
    }else if(ret_status_1 =! ESP_OK && ret_status_2 == ESP_OK){
        // writing INTR_STATUS_1 failed
        ESP_LOGE(TAG, ":INTR_STATUS_1 write failed");
        return ret_status_1;
    }
    
    ESP_LOGE(TAG, ":both Interrupt writes failed");
    return ESP_FAIL;
} 


static esp_err_t FIFO_config_init(i2c_master_dev_handle_t i2c_dev){
    return write(i2c_dev, FIFO_CONFIGURATION, FIFO_CONFIG_STATE);
}

static esp_err_t LED_curr_contr(i2c_master_dev_handle_t i2c_dev, uint8_t led_1, uint8_t led_2, uint8_t led_3, uint8_t led_4){
    ESP_ERR_CHECK(write(i2c_dev,LED1_PA, led_1));
    ESP_ERR_CHECK(write(i2c_dev,LED2_PA, led_2));
    ESP_ERR_CHECK(write(i2c_dev,LED3_PA, led_3));
    ESP_ERR_CHECK(write(i2c_dev,LED4_PA, led_4));

    return ESP_OK;
}
static esp_err_t set_mode(i2c_master_dev_handle_t i2c_dev, mode_t mode){
    if(mode!= HR_MODE||mode!= SPO2_MODE || mode!= MULTI_LED_MODE){
        ESP_LOGE(TAG, ": Mode not identified");
        return ESP_ERR_INVALID_ARG;
    }
    
    mode_t mode_state = mode;
    ESP_ERR_CHECK(write(i2c_dev, MODE_CONFIGURATION, mode_state));

    switch(mode){
        case HR_MODE:
        //LED current control config
            ESP_ERR_CHECK(
                LED_curr_contr(
                    i2c_dev, LED_CURR_CONTROL,LED_CURR_CONTROL,LED_CURR_CONTROL,LED_CURR_CONTROL
                )
            );
            // FIFO configuration
            ESP_ERR_CHECK(write(i2c_dev, FIFO_CONFIGURATION, FIFO_HR_MODE_CONFIG));
            break;


        case SPO2_MODE:
        //SPO configuration
        ESP_ERR_CHECK(write(i2c_dev, SPO2_CONFIGURATION, SP02_MODE_CONFIG));
        //LED current control config
        ESP_ERR_CHECK(
                LED_curr_contr(
                    i2c_dev, LED_CURR_CONTROL,LED_CURR_CONTROL,LED_CURR_CONTROL,LED_CURR_CONTROL
                )
            );
        // FIFO configuration
        ESP_ERR_CHECK(write(i2c_dev, FIFO_CONFIGURATION, FIFO_SPO2_MODE_CONFIG));
        break;



        case MULTI_LED_MODE:
        //MULTI-LED MODE
        ESP_ERR_CHECK(write(i2c_dev, MULTI_LED_MODE_CONTROL_REGISTERS1, MULTI_LED_MODE_CONFIG_1));
        ESP_ERR_CHECK(write(i2c_dev, MULTI_LED_MODE_CONTROL_REGISTERS2, MULTI_LED_MODE_CONFIG_2));
         //LED current control config
        ESP_ERR_CHECK(
                LED_curr_contr(
                    i2c_dev, LED_CURR_CONTROL,LED_CURR_CONTROL,LED_CURR_CONTROL,LED_CURR_CONTROL
                )
            );
        // FIFO configuration
        ESP_ERR_CHECK(write(i2c_dev, FIFO_CONFIGURATION, FIFO_MULTI_LED_MODE_CONFIG));
        break;      
    }  
    return ESP_OK;
}



esp_err_t read_FIFO_data(){
    //FIFO_ROLLOVER_EN = 0
    //READ FIFO_DATA into buffer
    // Increment read pointer
    //FIFO_ROLLOVER_EN = 1
    //FIFO_ROLLOVER_EN = 1 and read
}

esp_err_t MAX30101_initialize(){
    
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC,
        .i2c_port = I2C_PORT_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x57,
        .scl_speed_hz = SCL_SPEED,
    };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    //read PartID
    ESP_ERROR_CHECK(ID_initialize(dev_handle));

    //config Interrupt registers
    ESP_ERR_CHECK(intr_config(dev_handle));
    
    //config FIFO
    ESP_ERR_CHECK(FIFO_config_init(dev_handle));

    //CLEAR FIFO registers: ----------------------------
    write(dev_handle, FIFO_WRITE_POINTER,FIFO_POR_STATE);
    write(dev_handle, OVERFLOW_COUNTER,  FIFO_POR_STATE);
    write(dev_handle, FIFO_READ_POINTER, FIFO_POR_STATE);
    write(dev_handle, FIFO_DATA_POINTER, FIFO_POR_STATE);
    //---------------------------------------------------

    //Mode config HR
    ESP_ERR_CHECK(set_mode(dev_handle, HR_MODE));
    

    return ESP_OK;

}