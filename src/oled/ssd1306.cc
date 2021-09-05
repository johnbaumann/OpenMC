#include "oled/ssd1306.h"

#include "oled/6x6font.h"
#include "logging.h"
#include "pins.h"

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <string.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define GLYPH_WIDTH 6
#define GLYPH_HEIGHT 6

// References
// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf
// https://github.com/yanbe/ssd1306-esp-idf-i2c
// http://robotcantalk.blogspot.com/2015/03/interfacing-arduino-with-ssd1306-driven.html

namespace esp_sio_dev
{
	namespace oled
	{
		static bool display_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
		static uint8_t kSlaveAddress = 0x78;

		static esp_err_t SendCommandStream(uint8_t *command_buffer, size_t length);
		static esp_err_t SendSingleCommand(uint8_t command);

		void DrawChar(uint8_t character, int32_t x_pos, int32_t y_pos, bool clear_bg = false)
		{
			uint16_t glyph_index;

			if (character < 32 || character > 126)
			{
				glyph_index = 0;
			}
			else
			{
				glyph_index = (character - 32) * 6;
			}

			for (uint8_t byte_index = 0; byte_index < 6; byte_index++)
			{
				uint8_t _y = y_pos + byte_index;
				if (_y >= DISPLAY_HEIGHT)
					return;

				for (uint8_t bit = 7, x_offset = 0; bit >= 1; bit--, x_offset++)
				{
					int32_t _x = (x_pos + x_offset);
					if (_x >= DISPLAY_WIDTH || _x < 0)
						continue;

					if (clear_bg)
						display_buffer[(_y * DISPLAY_WIDTH) + _x] = ((font_6x6[glyph_index + byte_index] >> bit) & 0x01);
					else
						display_buffer[(_y * DISPLAY_WIDTH) + _x] |= ((font_6x6[glyph_index + byte_index] >> bit) & 0x01);
				}
			}
		}

		void DrawMessage(const char *message, int32_t x_pos, int32_t y_pos, bool auto_wrap, bool clear_bg = false)
		{
			int32_t _y = y_pos;
			int32_t _x = x_pos;

			for (uint16_t i = 0; i < (uint16_t)strlen(message); i++)
			{
				if (message[i] == '\n')
				{
					_y += GLYPH_HEIGHT + 1;
					_x = x_pos;
					continue;
				}

				if(auto_wrap && ((_x + GLYPH_WIDTH) >= DISPLAY_WIDTH))
				{
					_y += GLYPH_HEIGHT + 1;
					_x = x_pos;
				}

				DrawChar(message[i], _x, _y, clear_bg);
				_x += (GLYPH_WIDTH + 1);
			}
		}

		int32_t GetMessageRenderedWidth(const char *message)
		{
			uint16_t message_len = strlen(message);
			return (message_len * 6) + (1 * message_len);
		}

		static esp_err_t SendCommandStream(uint8_t *command_buffer, size_t length)
		{
			i2c_cmd_handle_t i2c_cmd_handle;
			esp_err_t esp_err;

			i2c_cmd_handle = i2c_cmd_link_create();
			i2c_master_start(i2c_cmd_handle);
			i2c_master_write_byte(i2c_cmd_handle, kSlaveAddress, true);
			i2c_master_write_byte(i2c_cmd_handle, ControlBytes::kCommandStream, true);

			i2c_master_write(i2c_cmd_handle, command_buffer, length, true);

			i2c_master_stop(i2c_cmd_handle);
			esp_err = i2c_master_cmd_begin(I2C_NUM_1, i2c_cmd_handle, 10 / portTICK_PERIOD_MS);
			i2c_cmd_link_delete(i2c_cmd_handle);

			return esp_err;
		}

		static esp_err_t SendSingleCommand(uint8_t command)
		{
			i2c_cmd_handle_t i2c_cmd_handle;
			esp_err_t esp_err;

			i2c_cmd_handle = i2c_cmd_link_create();
			i2c_master_start(i2c_cmd_handle);
			i2c_master_write_byte(i2c_cmd_handle, kSlaveAddress, true);
			i2c_master_write_byte(i2c_cmd_handle, ControlBytes::kSingleCommandByte, true);

			i2c_master_write_byte(i2c_cmd_handle, command, true);

			i2c_master_stop(i2c_cmd_handle);
			esp_err = i2c_master_cmd_begin(I2C_NUM_1, i2c_cmd_handle, 10 / portTICK_PERIOD_MS);
			i2c_cmd_link_delete(i2c_cmd_handle);

			return esp_err;
		}

		esp_err_t SetContrast(uint8_t contrast)
		{
			uint8_t command_list[2] = {
				Commands::kSetContrast,
				contrast};

			return SendCommandStream(command_list, 2);
		}

		// Displays from RAM, normal behavior
		esp_err_t DisplayFromRAM()
		{
			return SendSingleCommand(Commands::kDisplayFromRAM);
		}

		// All pixels "on"
		esp_err_t EntireDisplayOn()
		{
			return SendSingleCommand(Commands::kEntireDisplayOn);
		}

		// Normal display mode, set bits are illuminated
		esp_err_t NormalDisplayMode()
		{
			return SendSingleCommand(Commands::kNormalDisplayMode);
		}

		// Inverse display mode, clear bits are illuminated
		esp_err_t InverseDisplayMode()
		{
			return SendSingleCommand(Commands::kInverseDisplayMode);
		}

		// Turn display off
		esp_err_t DisplayOff()
		{
			return SendSingleCommand(Commands::kDisplayOff);
		}

		// Turn display on
		esp_err_t DisplayOn()
		{
			return SendSingleCommand(Commands::kDisplayOn);
		}

		// Scroll a range of the display right
		esp_err_t ScrollRight(Page start_page, Interval interval, Page end_page)
		{
			uint8_t command_list[7] = {
				Commands::kScrollRight,
				0x00, // Dummy byte
				start_page,
				interval,
				end_page,
				0x00, // Dummy byte
				0xFF  // Dummy byte
			};

			return SendCommandStream(command_list, 7);
		}

		// Scroll a range of the display left
		esp_err_t ScrollLeft(Page start_page, Interval interval, Page end_page)
		{
			uint8_t command_list[7] = {
				Commands::kScrollLeft,
				0x00, // Dummy byte
				start_page,
				interval,
				end_page,
				0x00, // Dummy byte
				0xFF  // Dummy byte
			};

			return SendCommandStream(command_list, 7);
		}

		// Scroll a range of the display down and right
		esp_err_t ScrollDownAndRight(Page start_page, Interval interval, Page end_page, uint8_t offset)
		{
			uint8_t command_list[6] = {
				Commands::kScrollDownAndRight,
				0x00, // Dummy byte
				start_page,
				interval,
				end_page,
				offset};

			return SendCommandStream(command_list, 7);
		}

		// Scroll a range of the display down and left
		esp_err_t ScrollDownAndLeft(Page start_page, Interval interval, Page end_page, uint8_t offset)
		{
			uint8_t command_list[6] = {
				Commands::kScrollDownAndLeft,
				0x00, // Dummy byte
				start_page,
				interval,
				end_page,
				offset};

			return SendCommandStream(command_list, 6);
		}

		//
		esp_err_t DeactivateScroll()
		{
			return SendSingleCommand(Commands::kDeactivateScroll);
		}

		esp_err_t ActivateScroll()
		{
			return SendSingleCommand(Commands::kActivateScroll);
		}

		esp_err_t SetVerticalScrollArea(uint8_t num_top_fix_rows, uint8_t num_scroll_rows)
		{
			uint8_t command_list[3] = {
				Commands::kSetVerticalScrollArea,
				num_top_fix_rows,
				num_scroll_rows};

			return SendCommandStream(command_list, 3);
		}

		void DrawTestPatternToBuffer()
		{
			// Diaganol 1
			for (uint8_t y = 0, x = 0; y < DISPLAY_HEIGHT; y++, x += 2)
			{
				//display_buffer[(DISPLAY_WIDTH * (y)) + x] = true;
			}

			// Diaganol 2
			for (uint8_t y = 0, x = DISPLAY_WIDTH; y < DISPLAY_HEIGHT; y++, x -= 2)
			{
				//display_buffer[(DISPLAY_WIDTH * (y)) + x] = true;
			}

			// Top
			for (uint8_t x = 0; x < DISPLAY_WIDTH; x++)
			{
				display_buffer[x] = true;
			}

			// Bottom
			for (uint8_t x = 0; x < DISPLAY_WIDTH; x++)
			{
				display_buffer[((DISPLAY_HEIGHT - 1) * DISPLAY_WIDTH) + x] = true;
			}

			// Left
			for (uint8_t y = 0; y < DISPLAY_HEIGHT; y++)
			{
				display_buffer[y * DISPLAY_WIDTH] = true;
			}

			// Right
			for (uint8_t y = 0; y < DISPLAY_HEIGHT; y++)
			{
				display_buffer[(y * DISPLAY_WIDTH) + (DISPLAY_WIDTH - 1)] = true;
			}

			DrawMessage("!\"#$%&'()*+,-./01", 2, 2, true, false);
			DrawMessage("23456789:;<=>?@AB", 2, 9, true, false);
			DrawMessage("CDEFGHIJKLMNOPQRS", 2, 16, true, false);
			DrawMessage("TUVWXYZ[\\]^_`abcd", 2, 23, true, false);
			DrawMessage("efghijklmnopqrstu", 2, 30, true, false);
			DrawMessage("vwxyz{|}~", 2, 37, true, false);
		}

		void ClearBuffer()
		{
			memset(display_buffer, 0x00, DISPLAY_WIDTH * DISPLAY_HEIGHT);
		}

		void ClearScreen()
		{
			ClearBuffer();
			DrawBuffer();
		}

		void DrawBuffer()
		{
			i2c_cmd_handle_t i2c_cmd_handle;

			// DISPLAY_WIDTH = 128
			// DISPLAY_HEIGHT = 64 = 8 * 8 bits

			// Pixels are bitpacked vertically, so divide height by 8
			for (uint8_t y = 0; y < (DISPLAY_HEIGHT / 8); y++)
			{
				i2c_cmd_handle = i2c_cmd_link_create();
				i2c_master_start(i2c_cmd_handle);
				i2c_master_write_byte(i2c_cmd_handle, kSlaveAddress, true);
				i2c_master_write_byte(i2c_cmd_handle, ControlBytes::kSingleCommandByte, true);
				i2c_master_write_byte(i2c_cmd_handle, 0xB0 | y, true);
				i2c_master_write_byte(i2c_cmd_handle, ControlBytes::kDataStream, true);
				for (uint8_t x = 0; x < DISPLAY_WIDTH; x++)
				{
					uint8_t out_byte = 0;
					for (uint8_t bit = 0; bit < 8; bit++)
					{
						if (display_buffer[(DISPLAY_WIDTH * ((y * 8) + bit)) + x])
						{
							out_byte |= (1 << bit);
						}
					}
					i2c_master_write_byte(i2c_cmd_handle, out_byte, true);
				}
				i2c_master_stop(i2c_cmd_handle);
				i2c_master_cmd_begin(I2C_NUM_1, i2c_cmd_handle, 10 / portTICK_PERIOD_MS);
				i2c_cmd_link_delete(i2c_cmd_handle);
			}
		}

		void Init(void)
		{
			uint8_t init_sequence[28] = {
				kSlaveAddress,
				ControlBytes::kCommandStream,
				Commands::kDisplayOff,
				Commands::kSetClockFrequency,
				0xF0, // Max frequency, no divide ratio. Bitmask = XXXX YYYY, X = FREQUENCY, Y = DIVIDE
				Commands::kSetMultiplexRatio,
				DISPLAY_HEIGHT - 1,
				Commands::kSetDisplayOffset,
				0,
				ControlBytes::kDataStream,
				Commands::kSetChargePump,
				0x14,
				Commands::kSetMemoryAddressingMode,
				MemoryAddressing::HorizontalMode,
				Commands::kSegmentReset,
				Commands::kCOMReset,
				Commands::kSetCOMPins,
				0x12,
				Commands::kSetContrast,
				0xFF,
				Commands::kSetPrechargePeriod,
				0xF1,
				Commands::kSetPreselectLevel,
				0x40,
				Commands::kDisplayFromRAM,
				Commands::kNormalDisplayMode,
				Commands::kDeactivateScroll,
				Commands::kDisplayOn};

			i2c_config_t i2c_config = {
				.mode = I2C_MODE_MASTER,
				.sda_io_num = kOLEDPin_SDA,
				.scl_io_num = kOLEDPin_SCL,
				.sda_pullup_en = GPIO_PULLUP_ENABLE,
				.scl_pullup_en = GPIO_PULLUP_ENABLE,
				.master = {.clk_speed = 700000}, // Clock speed set for 180Mhz ESP32 CPU Speed
				.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL};
			i2c_param_config(I2C_NUM_1, &i2c_config);
			i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0);

			ResetScreen();

			SendCommandStream(init_sequence, 28);

			SetContrast(0x00);

			ClearScreen();
			//ClearBuffer();
			//DrawTestPatternToBuffer();
			//DrawBuffer();
			//ScrollDownAndRight(Page::kPAGE0, Interval::k2Frames, Page::kPAGE7, 0x3F);
			//ActivateScroll();
		}

		void ResetScreen()
		{
			// Reset OLED
			gpio_set_direction(kOLEDPin_RST, GPIO_MODE_OUTPUT);
			gpio_set_level(kOLEDPin_RST, 0);
			vTaskDelay(pdMS_TO_TICKS(100));
			gpio_set_level(kOLEDPin_RST, 1);
		}
	} // oled
} // esp_sio_dev
