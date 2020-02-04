#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "driver/i2c.h"
#include "esp_console.h"
#include "esp_log.h"
#include "cmd_ledtools.h"

/* timer task */
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

#define I2C_MASTER_TX_BUF_DISABLE 0     /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0     /*!< I2C master doesn't need buffer */
#define WRITE_BIT I2C_MASTER_WRITE      /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ        /*!< I2C master read */
#define ACK_CHECK_EN 0x1                /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0               /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                     /*!< I2C ack value */
#define NACK_VAL 0x1                    /*!< I2C nack value */
#define PCA9570_ADDR 0x24               /*!< PCA9570 address */

#define G_pos 0                         /*!< PCA9570_P0 -> green led */
#define B_pos 1                         /*!< PCA9570_P1 -> blue led */
#define R_pos 2                         /*!< PCA9570_P2 -> red led */
#define G_off   (1<<G_pos)              /*!< turn off led green */
#define B_off   (1<<B_pos)              /*!< turn off led blue */
#define R_off   (1<<R_pos)              /*!< turn off led red */
#define G_shine (~G_off)                /*!< turn on led green */
#define B_shine (~B_off)                /*!< turn on led blue */
#define R_shine (~R_off)                /*!< turn on led red */
#define DEMO 1                          /*!< enable demo */
#define ms_2_us(x) (x*1000)             /*!< convert ms to us */

#define MAX_CMD_NUM 7                   /*!< number of supported command */
const char *demo_cmd[MAX_CMD_NUM] = {
                "blink red 500",
                "blink green 500",
                "blink blue 500",
                "shine red",
                "shine green",
                "shine blue",
                "cancel"
};

const char *color_str[MAX_COLOR] = {
                "green",
                "blue",
                "red"
};

static const char *TAG = "PCA9570";
static gpio_num_t i2c_gpio_sda = 12;
static gpio_num_t i2c_gpio_scl = 4;
static uint32_t i2c_frequency = 1000000;
static i2c_port_t i2c_port = I2C_NUM_0;

static uint16_t demo_cmd_cnt = 0;
static uint8_t demo_cmd_idx = MAX_CMD_NUM;

static bool blink_timer_running = false;
static bool demo_timer_running = false;

esp_timer_handle_t blink_timer_handler;
esp_timer_handle_t demo_timer_handler;

static struct {
        struct arg_str *color;
        struct arg_int *parameter;
        struct arg_end *end;
} ledblink_args;

static struct {
        struct arg_str *color;
        struct arg_end *end;
} ledshine_args;

static struct {
        struct arg_str *color;
        struct arg_end *end;
} ledoff_args;

esp_err_t i2c_master_driver_initialize(void)
{
        i2c_config_t conf = {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = i2c_gpio_sda,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_io_num = i2c_gpio_scl,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master.clk_speed = i2c_frequency
        };
        return i2c_param_config(i2c_port, &conf);
}

esp_err_t pca9570_read_byte(uint8_t* data)
{
        i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
        i2c_master_driver_initialize();
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, PCA9570_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
        i2c_master_read_byte(cmd, data, NACK_VAL);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        i2c_driver_delete(i2c_port);
        return ret;
}

esp_err_t pca9570_write_byte(uint8_t data)
{
        i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
        i2c_master_driver_initialize();
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, PCA9570_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        i2c_driver_delete(i2c_port);
        return ret;
}

esp_err_t cmd_get_color(const char *cmd_str, uint8_t *color_ptr)
{
        if (!strcmp(color_str[GREEN],cmd_str)) {
                *color_ptr = GREEN;
        } else if (!strcmp(color_str[BLUE],cmd_str)) {
                *color_ptr = BLUE;
        } else if (!strcmp(color_str[RED],cmd_str)) {
                *color_ptr = RED;
        } else {
                return ESP_FAIL;
        }
        return ESP_OK;
}

void terminate_timer(uint8_t timer)
{
        switch (timer) {
        case BLINK_TIMER:
                if(blink_timer_running) {
                        // Clean up and finish timer
                        ESP_ERROR_CHECK(esp_timer_stop(blink_timer_handler));
                        ESP_ERROR_CHECK(esp_timer_delete(blink_timer_handler));
                        blink_timer_running = false;
                }
                break;
        case DEMO_TIMER:
                if(demo_timer_running) {
                        // Clean up and finish timer
                        ESP_ERROR_CHECK(esp_timer_stop(demo_timer_handler));
                        ESP_ERROR_CHECK(esp_timer_delete(demo_timer_handler));
                        demo_timer_running = false;
                }
                break;
        default:
                break;
        }
}

void start_timer(uint8_t timer, int64_t interval_ms)
{
        switch (timer) {
        case BLINK_TIMER:
                if (blink_timer_running)
                        terminate_timer(timer);

                const esp_timer_create_args_t timer0_args = {
                        .callback = &blink_timer_callback,
                        .name = "blink timer"
                };
                ESP_ERROR_CHECK(esp_timer_create(&timer0_args, &blink_timer_handler));
                ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer_handler, ms_2_us(interval_ms)));
                blink_timer_running = true;
                break;
        case DEMO_TIMER:
                if (demo_timer_running)
                        terminate_timer(timer);

                const esp_timer_create_args_t timer1_args = {
                        .callback = &demo_timer_callback,
                        .name = "demo timer"
                };
                ESP_ERROR_CHECK(esp_timer_create(&timer1_args, &demo_timer_handler));
                ESP_ERROR_CHECK(esp_timer_start_periodic(demo_timer_handler, ms_2_us(interval_ms)));
                demo_timer_running = true;
                break;
        default:
                break;
        }
}

esp_err_t led_toggle(uint8_t color)
{
        uint8_t data = 0;

        esp_err_t ret = pca9570_read_byte(&data);
        if (ret != ESP_OK)
                return ret;

        switch (color) {
        case GREEN:
                data ^= 1 << G_pos;
                break;
        case BLUE:
                data ^= 1 << B_pos;
                break;
        case RED:
                data ^= 1 << R_pos;
                break;
        default:
                return ESP_FAIL;
                break;
        }
        return pca9570_write_byte(data);
}

esp_err_t led_on(uint8_t color)
{
        uint8_t data = 0;

        esp_err_t ret = pca9570_read_byte(&data);
        if (ret != ESP_OK)
                return ret;

        switch (color) {
        case GREEN:
                data &= G_shine;
                break;
        case BLUE:
                data &= B_shine;
                break;
        case RED:
                data &= R_shine;
                break;
        default:
                return ESP_FAIL;
                break;
        }
        return pca9570_write_byte(data);
}

esp_err_t led_off(uint8_t color)
{
        uint8_t data = 0;

        esp_err_t ret = pca9570_read_byte(&data);
        if (ret != ESP_OK)
                return ret;

        switch (color) {
        case GREEN:
                data |= G_off;
                break;
        case BLUE:
                data |= B_off;
                break;
        case RED:
                data |= R_off;
                break;
        default:
                return ESP_FAIL;
                break;
        }
        return pca9570_write_byte(data);
}

esp_err_t do_blink_cmd(uint8_t *color_ptr, int64_t blink_ms)
{
        if (*color_ptr >= MAX_COLOR)
                return ESP_FAIL;

        if ((blink_ms >= MAX_BLINK_PARAM) || (blink_ms <= MIN_BLINK_PARAM))
                return ESP_FAIL;
#ifdef DEMO
        demo_cmd_cnt += 1;
#endif
#ifdef CANCEL_ALL
        // cancel current cmd by turn off all LED if any
        led_off(GREEN);
        led_off(BLUE);
        led_off(RED);
#endif
        start_timer(BLINK_TIMER,blink_ms);
        return led_toggle(*color_ptr);
}

esp_err_t do_shine_cmd(uint8_t *color_ptr)
{
        if (*color_ptr >= MAX_COLOR)
                return ESP_FAIL;

#ifdef DEMO
        demo_cmd_cnt += 1;
#endif
        terminate_timer(BLINK_TIMER);
#ifdef CANCEL_ALL
        // cancel current cmd by turn off all LED if any
        ESP_ERROR_CHECK(led_off(GREEN));
        ESP_ERROR_CHECK(led_off(BLUE));
        ESP_ERROR_CHECK(led_off(RED));
#endif
        return led_on(*color_ptr);
}

esp_err_t do_off_cmd(uint8_t *color_ptr)
{
        if (*color_ptr >= MAX_COLOR)
                return ESP_FAIL;

        terminate_timer(BLINK_TIMER);
#ifdef CANCEL_ALL
        // cancel current cmd by turn off all LED if any
        ESP_ERROR_CHECK(led_off(GREEN));
        ESP_ERROR_CHECK(led_off(BLUE));
        ESP_ERROR_CHECK(led_off(RED));
#endif
        return led_off(*color_ptr);
}

void do_cancel_cmd(void)
{
        if (blink_timer_running)
                terminate_timer(BLINK_TIMER);

        if (demo_timer_running)
                terminate_timer(DEMO_TIMER);

        ESP_ERROR_CHECK(led_off(GREEN));
        ESP_ERROR_CHECK(led_off(BLUE));
        ESP_ERROR_CHECK(led_off(RED));
}

void do_demo_cmd(void)
{
        int ret;
        start_timer(DEMO_TIMER,DEMO_TIMEOUT);
        demo_cmd_idx = 0;
        demo_cmd_cnt = 0;
        ESP_ERROR_CHECK(esp_console_run(demo_cmd[demo_cmd_idx], &ret));
}

void blink_timer_callback(void* arg)
{
        ESP_ERROR_CHECK(led_toggle(cmd_color));
}

void demo_timer_callback(void* arg)
{
        if (demo_cmd_idx < MAX_CMD_NUM) {
                demo_cmd_idx += 1;
                if (demo_cmd_idx != demo_cmd_cnt) {
                        //new command inserted, terminate demo
                        terminate_timer(DEMO_TIMER);
                        return;
                } else {
                        //continue running demo
                        int ret;
                        ESP_ERROR_CHECK(esp_console_run(demo_cmd[demo_cmd_idx], &ret));
                }
        } else {
                // end demo
                do_cancel_cmd();
        }
}

/***********************************/
/****commands callback function*****/
/***********************************/
int cmd_blink_callback(int argc, char **argv)
{
        esp_err_t ret;
        int nerrors = arg_parse(argc, argv, (void **)&ledblink_args);
        if (nerrors != 0) {
                arg_print_errors(stderr, ledblink_args.end, argv[0]);
                return nerrors;
        }

        // Check color input option
        cmd_color = MAX_COLOR;
        ret = cmd_get_color(ledblink_args.color->sval[0], &cmd_color);
        if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Invalid color input: %s",ledblink_args.color->sval[0]);
                return nerrors;
        }

        // Check color input option
        int64_t parameter = 0;
        if (ledblink_args.parameter->count)
                parameter = ledblink_args.parameter->ival[0];

        if ((parameter >= MAX_BLINK_PARAM) || (parameter <= MIN_BLINK_PARAM)) {
                ESP_LOGE(TAG, "Parameter out of range (%d/%d): %d",MAX_BLINK_PARAM,MIN_BLINK_PARAM,ledblink_args.parameter->ival[0]);
                return nerrors;
        }

        ret = do_blink_cmd(&cmd_color, parameter);
        if (ret != ESP_OK)
                ESP_LOGE(TAG, "Fail to execute blink command");

        return 0;
}

int cmd_shine_callback(int argc, char **argv)
{
        esp_err_t ret;
        int nerrors = arg_parse(argc, argv, (void **)&ledshine_args);
        if (nerrors != 0) {
                arg_print_errors(stderr, ledshine_args.end, argv[0]);
                return nerrors;
        }

        // Check color input option
        cmd_color = MAX_COLOR;
        ret = cmd_get_color(ledshine_args.color->sval[0], &cmd_color);
        if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Invalid color input: %s",ledshine_args.color->sval[0]);
                return nerrors;
        }

        ret = do_shine_cmd(&cmd_color);
        if (ret != ESP_OK)
                ESP_LOGE(TAG, "Fail to execute shine command");

        return 0;
}

int cmd_off_callback(int argc, char **argv)
{
        esp_err_t ret;
        int nerrors = arg_parse(argc, argv, (void **)&ledoff_args);
        if (nerrors != 0) {
                arg_print_errors(stderr, ledoff_args.end, argv[0]);
                return nerrors;
        }

        // Check color input option
        cmd_color = MAX_COLOR;
        ret = cmd_get_color(ledoff_args.color->sval[0], &cmd_color);
        if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Invalid color input: %s",ledoff_args.color->sval[0]);
                return nerrors;
        }

        ret = do_off_cmd(&cmd_color);
        if (ret != ESP_OK)
                ESP_LOGE(TAG, "Fail to execute off command");

        return 0;
}

int cmd_cancel_callback(int argc, char **argv)
{
        if (argc > 1)
                ESP_LOGE(TAG, "No argument allowed!!!");
        else
                do_cancel_cmd();

        return 0;
}

int cmd_demo_callback(int argc, char **argv)
{
        if (argc > 1)
                ESP_LOGE(TAG, "No argument allowed!!!");
        else
                do_demo_cmd();

        return 0;
}

/***********************************/
/****** register commands here *****/
/***********************************/
void register_led_blink(void)
{
        ledblink_args.color = arg_str0(NULL, NULL, "<color>", " green, red or blue");
        ledblink_args.parameter = arg_int0(NULL, NULL, "<parameter>", " blinking interval in ms");
        ledblink_args.end = arg_end(1);
        const esp_console_cmd_t led_blink_cmd = {
                .command = "blink",
                .help = "blink red/green/blue LED",
                .hint = NULL,
                .func = &cmd_blink_callback,
                .argtable = &ledblink_args
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&led_blink_cmd));
}

void register_led_shine(void)
{
        ledshine_args.color = arg_str0(NULL, NULL, "<color>", " green, red or blue");
        ledshine_args.end = arg_end(1);
        const esp_console_cmd_t led_shine_cmd = {
                .command = "shine",
                .help = "shine red/green/blue LED",
                .hint = NULL,
                .func = &cmd_shine_callback,
                .argtable = &ledshine_args
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&led_shine_cmd));
}

void register_led_off(void)
{
        ledoff_args.color = arg_str0(NULL, NULL, "<color>", " green, red or blue");
        ledoff_args.end = arg_end(1);
        const esp_console_cmd_t led_off_cmd = {
                .command = "off",
                .help = "turn off red/green/blue LED",
                .hint = NULL,
                .func = &cmd_off_callback,
                .argtable = &ledoff_args
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&led_off_cmd));
}

void register_led_cancel(void)
{
        const esp_console_cmd_t led_cancel_cmd = {
                .command = "cancel",
                .help = "set all LEDs off",
                .hint = NULL,
                .func = &cmd_cancel_callback,
                .argtable = NULL
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&led_cancel_cmd));
}

void register_led_demo(void)
{
        const esp_console_cmd_t led_demo_cmd = {
                .command = "demo",
                .help = "run demo for blink, shine and off",
                .hint = NULL,
                .func = &cmd_demo_callback,
                .argtable = NULL
        };
        ESP_ERROR_CHECK(esp_console_cmd_register(&led_demo_cmd));
}

void register_leds(void)
{
        register_led_blink();
        register_led_shine();
        register_led_off();
        register_led_cancel();
        register_led_demo();
}
