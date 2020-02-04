#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#define BLINK_TIMER 0                   /*!< timer for blink LED */
#define DEMO_TIMER 1                    /*!< timer for running demo */
#define DEMO_TIMEOUT 3000               /*!< demo timeout for each command */
#define CANCEL_ALL 1                    /*!< cancel current cmd means turn of all led if any */
#define MAX_BLINK_PARAM 10000           /*!< maximum blink parameter allowed */
#define MIN_BLINK_PARAM 0               /*!< minimum blink parameter allowed */
#define GREEN 0                         /*!< color green index */
#define BLUE 1                          /*!< color blue index */
#define RED 2                           /*!< color red index */
#define MAX_COLOR 3                     /*!< green, blue, red */

uint8_t cmd_color;

esp_err_t i2c_master_driver_initialize(void);
esp_err_t pca9570_read_byte(uint8_t *data);
esp_err_t pca9570_write_byte(uint8_t data);
esp_err_t cmd_get_color(const char *cmd_str, uint8_t *color_ptr);
esp_err_t led_toggle(uint8_t color);
esp_err_t led_on(uint8_t color);
esp_err_t led_off(uint8_t color);
esp_err_t do_blink_cmd(uint8_t *color_ptr, int64_t blink_ms);
esp_err_t do_shine_cmd(uint8_t *color_ptr);
esp_err_t do_off_cmd(uint8_t *color_ptr);
void do_cancel_cmd(void);
void do_demo_cmd(void);

int cmd_blink_callback(int argc, char **argv);
int cmd_shine_callback(int argc, char **argv);
int cmd_off_callback(int argc, char **argv);
int cmd_cancel_callback(int argc, char **argv);
int cmd_demo_callback(int argc, char **argv);

void register_leds(void);
void register_led_shine(void);
void register_led_off(void);
void register_led_cancel(void);
void register_led_demo(void);

void blink_timer_callback(void* arg);
void demo_timer_callback(void* arg);
void terminate_timer(uint8_t timer);
void start_timer(uint8_t timer, int64_t interval_ms);

#ifdef __cplusplus
}
#endif