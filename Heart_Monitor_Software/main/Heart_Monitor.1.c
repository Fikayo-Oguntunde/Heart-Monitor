#include <stdio.h>
#include "MAX30101_driver.h"


void app_main(void)
{
    ESP_ERROR_CHECK(MAX30101_initialize());
}
