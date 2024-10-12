#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOSConfig.h"

#define BUF_SIZE        1024
#define BUF_SIZE_2      2048
#define ZERO            0
#define MUTEX_WAIT_TIME 100
#define Second_Delay    1000
#define Delay_500MS     500
#define Logic_High      1
#define Logic_Low       0

/* Global mutex handle */
SemaphoreHandle_t g_BinarySemaphore;

/* Function to configure the UART */
void ConfigureSerial(void)
{
    uart_config_t uart_settings = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_0, &uart_settings); /* Configure UART with specified settings */
    uart_driver_install(UART_NUM_0, BUF_SIZE_2, ZERO, ZERO, NULL, ZERO);
}

/* Function to configure GPIO */
void ConfigureGPIO(void)
{
    gpio_config_t gpio_settings = {
        .mode         = GPIO_MODE_DEF_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << GPIO_NUM_2) /* Configure GPIO2 */
    };
    gpio_config(&gpio_settings); /* Apply GPIO configuration */
}

/* Task 1: Turn LED on */
void Task1(void *pv_parameters)
{
    while (1)
    {
        if (xSemaphoreTake(g_BinarySemaphore, (TickType_t) MUTEX_WAIT_TIME) == pdTRUE)
        {
            /* Set GPIO2 high to turn LED on */
            gpio_set_level(GPIO_NUM_2, Logic_High);
            vTaskDelay(Delay_500MS / portTICK_PERIOD_MS); /* Delay for 500ms */
            xSemaphoreGive(g_BinarySemaphore);
        }
        else
        {
            printf("No MUTEX Received\n"); /* Print message if mutex is not available */
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Small delay to prevent tight looping
    }
}

/* Task 2: Turn LED off */
void Task2(void *pv_parameters)
{
    while (1)
    {
        if (xSemaphoreTake(g_BinarySemaphore, (TickType_t) MUTEX_WAIT_TIME) == pdTRUE)
        {
            /* Set GPIO2 low to turn LED off */
            gpio_set_level(GPIO_NUM_2, Logic_Low);
            vTaskDelay(Second_Delay / portTICK_PERIOD_MS); /* Delay for 1 second */
            xSemaphoreGive(g_BinarySemaphore);
        }
        else
        {
            printf("No Mutex Received\n");
        }

        vTaskDelay(100 / portTICK_PERIOD_MS); // Small delay to prevent tight looping
    }
}

/* Task 3: Print a status message */
void Task3(void *pv_parameters)
{
    while (1)
    {
        /* Print status message via UART */
        printf("Task 3: Status message\n");
        vTaskDelay(Second_Delay / portTICK_PERIOD_MS); /* Delay for 1 second */
    }
}

/* Main application entry point */
void app_main(void)
{
    ConfigureGPIO(); /* Configure GPIO */
    ConfigureSerial(); /* Configure UART */

    const int Priority_task_1 = 1; // Task one priority
    const int Priority_task_2 = 2; // Task two priority
    const int Priority_task_3 = 3; // Task three priority

    g_BinarySemaphore = xSemaphoreCreateBinary(); /* Create a binary semaphore */

    if (g_BinarySemaphore != NULL)
    {
        xSemaphoreGive(g_BinarySemaphore); // Give the semaphore initially to allow access

        /* Create tasks with different priorities */
        xTaskCreate(Task1, "Task 1", BUF_SIZE_2, NULL, Priority_task_1, NULL);
        xTaskCreate(Task2, "Task 2", BUF_SIZE_2, NULL, Priority_task_2, NULL);
        xTaskCreate(Task3, "Task 3", BUF_SIZE_2, NULL, Priority_task_3, NULL);
    }
}