#include <limits.h>
#include "unity.h"
#include "cmd_ledtools.h"
#include "esp_err.h"

#define GREEN 0                         /*!< color green index */
#define BLUE 1                          /*!< color blue index */
#define RED 2                           /*!< color red index */
#define MAX_COLOR 3                     /*!< green, blue, red */

/*************************************************************/
/************ test PCA9570 read/write byte function **********/
/*************************************************************/

TEST_CASE("PCA9570 read/write byte", "[PCA9570]")
{
        //write one by to PCA9570 first
        uint8_t data = 0b111;
        esp_err_t ret = pca9570_write_byte(data);
        if (ret == ESP_OK){
                ret = pca9570_read_byte(&data);
                TEST_ASSERT_EQUAL(0b111, 0b00000111 & data);
        }

        data = 0;
        ret = pca9570_write_byte(data);
        if (ret == ESP_OK){
                ret = pca9570_read_byte(&data);
                TEST_ASSERT_EQUAL(0, 0b00000111 & data);
        }

        data = 0b001;
        ret = pca9570_write_byte(data);
        if (ret == ESP_OK){
                ret = pca9570_read_byte(&data);
                TEST_ASSERT_EQUAL(0b001, 0b00000111 & data);
        }

        data = 0b010;
        ret = pca9570_write_byte(data);
        if (ret == ESP_OK){
                ret = pca9570_read_byte(&data);
                TEST_ASSERT_EQUAL(0b010, 0b00000111 & data);
        }

        data = 0b100;
        ret = pca9570_write_byte(data);
        if (ret == ESP_OK){
                ret = pca9570_read_byte(&data);
                TEST_ASSERT_EQUAL(0b100, 0b00000111 & data);
        }
}

/*************************************************************/
/***************** test cmd_get_color function ***************/
/*************************************************************/

TEST_CASE("PCA9570 test cmd_get_color() ", "[PCA9570][pass]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_OK,cmd_get_color("green",&color));
        TEST_ASSERT_EQUAL(GREEN,color);

        color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_OK,cmd_get_color("red",&color));
        TEST_ASSERT_EQUAL(RED,color);

        color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_OK,cmd_get_color("blue",&color));
        TEST_ASSERT_EQUAL(BLUE,color);
}

TEST_CASE("PCA9570 test cmd_get_color() ", "[PCA9570][fails]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_FAIL,cmd_get_color("bludidasoje",&color));
}

/*************************************************************/
/***************** test led_toggle function ******************/
/*************************************************************/

TEST_CASE("PCA9570 test led_toggle() ", "[PCA9570][pass]")
{
        uint8_t color = GREEN;
        TEST_ASSERT_EQUAL(ESP_OK,led_toggle(color));
        color = BLUE;
        TEST_ASSERT_EQUAL(ESP_OK,led_toggle(color));
        color = RED;
        TEST_ASSERT_EQUAL(ESP_OK,led_toggle(color));
}

TEST_CASE("PCA9570 test led_toggle() ", "[PCA9570][fails]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_FAIL,led_toggle(color));
}

/*************************************************************/
/***************** test led_on function **********************/
/*************************************************************/

TEST_CASE("PCA9570 test led_on() ", "[PCA9570][pass]")
{
        uint8_t color = GREEN;
        TEST_ASSERT_EQUAL(ESP_OK,led_on(color));
        color = BLUE;
        TEST_ASSERT_EQUAL(ESP_OK,led_on(color));
        color = RED;
        TEST_ASSERT_EQUAL(ESP_OK,led_on(color));
}

TEST_CASE("PCA9570 test led_on() ", "[PCA9570][fails]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_FAIL,led_on(color));
}

/*************************************************************/
/***************** test led_off function *********************/
/*************************************************************/

TEST_CASE("PCA9570 test led_off() ", "[PCA9570][pass]")
{
        uint8_t color = GREEN;
        TEST_ASSERT_EQUAL(ESP_OK,led_off(color));
        color = BLUE;
        TEST_ASSERT_EQUAL(ESP_OK,led_off(color));
        color = RED;
        TEST_ASSERT_EQUAL(ESP_OK,led_off(color));
}

TEST_CASE("PCA9570 test led_off() ", "[PCA9570][fails]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_FAIL,led_off(color));
}

/*************************************************************/
/***************** test do_blink_cmd function ****************/
/*************************************************************/

TEST_CASE("PCA9570 test do_blink_cmd() ", "[PCA9570][pass]")
{
        uint8_t color = GREEN;
        int64_t blink_ms = 1000;

        cmd_color = color;
        TEST_ASSERT_EQUAL(ESP_OK,do_blink_cmd(&color,blink_ms));

        color = BLUE;
        cmd_color = color;
        TEST_ASSERT_EQUAL(ESP_OK,do_blink_cmd(&color,blink_ms));

        color = RED;
        cmd_color = color;
        TEST_ASSERT_EQUAL(ESP_OK,do_blink_cmd(&color,blink_ms));

        color = MAX_COLOR;
        cmd_color = color;
        TEST_ASSERT_EQUAL(ESP_FAIL,do_blink_cmd(&color,blink_ms));
        terminate_timer(BLINK_TIMER);
}

TEST_CASE("PCA9570 test do_blink_cmd() ", "[PCA9570][fails]")
{
        uint8_t color = GREEN;
        int64_t blink_ms = MAX_BLINK_PARAM;
        TEST_ASSERT_EQUAL(ESP_FAIL,do_blink_cmd(&color,blink_ms));
}

/*************************************************************/
/***************** test do_shine_cmd function ****************/
/*************************************************************/

TEST_CASE("PCA9570 test do_shine_cmd() ", "[PCA9570][pass]")
{
        uint8_t color = GREEN;

        TEST_ASSERT_EQUAL(ESP_OK,do_shine_cmd(&color));

        color = BLUE;
        TEST_ASSERT_EQUAL(ESP_OK,do_shine_cmd(&color));

        color = RED;
        TEST_ASSERT_EQUAL(ESP_OK,do_shine_cmd(&color));
}

TEST_CASE("PCA9570 test do_shine_cmd() ", "[PCA9570][fails]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_FAIL,do_shine_cmd(&color));
}

/*************************************************************/
/***************** test do_shine_cmd function ****************/
/*************************************************************/

TEST_CASE("PCA9570 test do_off_cmd() ", "[PCA9570][pass]")
{
        uint8_t color = GREEN;

        TEST_ASSERT_EQUAL(ESP_OK,do_off_cmd(&color));

        color = BLUE;
        TEST_ASSERT_EQUAL(ESP_OK,do_off_cmd(&color));

        color = RED;
        TEST_ASSERT_EQUAL(ESP_OK,do_off_cmd(&color));
}

TEST_CASE("PCA9570 test do_off_cmd() ", "[PCA9570][fails]")
{
        uint8_t color = MAX_COLOR;
        TEST_ASSERT_EQUAL(ESP_FAIL,do_off_cmd(&color));
}