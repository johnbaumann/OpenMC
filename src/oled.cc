#include "pins.h"
#include "ssd1366.h"
#include "font8x8_basic.h"

#include <string.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"

// https://github.com/yanbe/ssd1306-esp-idf-i2c

/*#define SDA_PIN GPIO_NUM_4
#define SCL_PIN GPIO_NUM_15
#define RST_PIN GPIO_NUM_16
#define LED_PIN GPIO_NUM_25*/

#define tag "SSD1306"

namespace esp_sio_dev
{
	namespace oled
	{
		void Init()
		{
			esp_err_t espRc;

			i2c_config_t i2c_config = {
				.mode = I2C_MODE_MASTER,
				.sda_io_num = kOLEDPin_SDA,
				.scl_io_num = kOLEDPin_SCL,
				.sda_pullup_en = GPIO_PULLUP_ENABLE,
				.scl_pullup_en = GPIO_PULLUP_ENABLE,
				.master = {.clk_speed = 700000},
				.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL};
			i2c_param_config(I2C_NUM_1, &i2c_config);
			i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0);

			i2c_cmd_handle_t cmd = i2c_cmd_link_create();

			// Reset OLED
			gpio_set_direction(kOLEDPin_RST, GPIO_MODE_OUTPUT);
			gpio_set_level(kOLEDPin_RST, 0);
			vTaskDelay(pdMS_TO_TICKS(100));
			gpio_set_level(kOLEDPin_RST, 1);

			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
			i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_OFF, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_CLK_DIV, true);
			i2c_master_write_byte(cmd, 0xF0, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_MUX_RATIO, true);
			i2c_master_write_byte(cmd, 64 - 1, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_OFFSET, true);
			i2c_master_write_byte(cmd, 0, true);
			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP, true);
			i2c_master_write_byte(cmd, 0x14, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_MEMORY_ADDR_MODE, true);
			i2c_master_write_byte(cmd, 0x00, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_COM_SCAN_MODE, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_COM_PIN_MAP, true);
			i2c_master_write_byte(cmd, 0x12, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
			i2c_master_write_byte(cmd, 0xCF, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_PRECHARGE, true);
			i2c_master_write_byte(cmd, 0xF1, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_VCOMH_DESELCT, true);
			i2c_master_write_byte(cmd, 0x40, true);
			i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_RAM, true);
			i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_NORMAL, true);
			i2c_master_write_byte(cmd, 0x2E, true);

			i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_ON, true);
			i2c_master_stop(cmd);

			espRc = i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
			if (espRc == ESP_OK)
			{
				ESP_LOGI(tag, "OLED configured successfully");
			}
			else
			{
				ESP_LOGE(tag, "OLED configuration failed. code: 0x%.2X", espRc);
			}
			i2c_cmd_link_delete(cmd);
		}

		void Clear()
		{
			i2c_cmd_handle_t cmd;

			for (uint8_t i = 0; i < 8; i++)
			{
				cmd = i2c_cmd_link_create();
				i2c_master_start(cmd);
				i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
				i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
				i2c_master_write_byte(cmd, 0xB0 | i, true);
				i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
				for (uint8_t j = 0; j < 128; j++)
				{
					i2c_master_write_byte(cmd, 0, true);
				}
				i2c_master_stop(cmd);
				i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
				i2c_cmd_link_delete(cmd);
			}
		}

		void SetContrast(uint8_t contrast)
		{
			i2c_cmd_handle_t cmd;

			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
			i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
			i2c_master_write_byte(cmd, contrast, true);
			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
		}

		void task_ssd1306_scroll(void *ignore)
		{
			esp_err_t espRc;

			i2c_cmd_handle_t cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);

			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

			i2c_master_write_byte(cmd, 0x29, true); // vertical and horizontal scroll (p29)
			i2c_master_write_byte(cmd, 0x00, true);
			i2c_master_write_byte(cmd, 0x00, true);
			i2c_master_write_byte(cmd, 0x07, true);
			i2c_master_write_byte(cmd, 0x01, true);
			i2c_master_write_byte(cmd, 0x3F, true);

			i2c_master_write_byte(cmd, 0xA3, true); // set vertical scroll area (p30)
			i2c_master_write_byte(cmd, 0x20, true);
			i2c_master_write_byte(cmd, 0x40, true);

			i2c_master_write_byte(cmd, 0x2F, true); // activate scroll (p29)

			i2c_master_stop(cmd);
			espRc = i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
			if (espRc == ESP_OK)
			{
				ESP_LOGI(tag, "Scroll command succeeded");
			}
			else
			{
				ESP_LOGE(tag, "Scroll command failed. code: 0x%.2X", espRc);
			}

			i2c_cmd_link_delete(cmd);
		}

		void display_text(void *arg_text)
		{
			char *text = (char *)arg_text;
			uint8_t text_len = strlen(text);

			i2c_cmd_handle_t cmd;

			uint8_t cur_page = 0;

			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
			i2c_master_write_byte(cmd, 0x00, true); // reset column
			i2c_master_write_byte(cmd, 0x10, true);
			i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);

			for (uint8_t i = 0; i < text_len; i++)
			{
				if (text[i] == '\n')
				{
					cmd = i2c_cmd_link_create();
					i2c_master_start(cmd);
					i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

					i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
					i2c_master_write_byte(cmd, 0x00, true); // reset column
					i2c_master_write_byte(cmd, 0x10, true);
					i2c_master_write_byte(cmd, 0xB0 | ++cur_page, true); // increment page

					i2c_master_stop(cmd);
					i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
					i2c_cmd_link_delete(cmd);
				}
				else
				{
					cmd = i2c_cmd_link_create();
					i2c_master_start(cmd);
					i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

					i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
					i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)text[i]], 8, true);

					i2c_master_stop(cmd);
					i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
					i2c_cmd_link_delete(cmd);
				}
			}
		}

		void init_oled(void)
		{
			Init();

			Clear();
			display_text((void *)"Hello world!\nMulitine is OK!\nAnother line");
		}
	} // oled
} // esp_sio_dev
