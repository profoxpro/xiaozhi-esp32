#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define ESP32_I2C_HOST      I2C_NUM_0
#define ESP32_LCD_HOST      SPI3_HOST

// Audio configuration
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_INPUT_REFERENCE    true

#define AUDIO_I2S_GPIO_MCLK  GPIO_NUM_4
#define AUDIO_I2S_GPIO_WS    GPIO_NUM_7
#define AUDIO_I2S_GPIO_BCLK  GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN   GPIO_NUM_6   // Исправлено: вход с GPIO 6
#define AUDIO_I2S_GPIO_DOUT  GPIO_NUM_8   // Исправлено: выход на GPIO 8

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_1
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_16
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_15
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

#define BOOT_BUTTON_GPIO        GPIO_NUM_0

// ILI9341V Display configuration (из спецификации)
#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_12
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_11
#define DISPLAY_SPI_CS_PIN      GPIO_NUM_10
#define DISPLAY_DC_PIN          GPIO_NUM_46
#define DISPLAY_RST_PIN         GPIO_NUM_NC  // Not using hardware reset to avoid conflict with BOOT button

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y true
#define DISPLAY_SWAP_XY  false

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_45
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// FT6X36 Touch controller (из спецификации)
#define TOUCH_I2C_SDA_PIN    GPIO_NUM_16
#define TOUCH_I2C_SCL_PIN    GPIO_NUM_15
#define TOUCH_I2C_ADDR       0x38
#define TOUCH_RST_PIN        GPIO_NUM_18
#define TOUCH_INT_PIN        GPIO_NUM_17

// SD Card (SDIO из спецификации)
#define SD_MMC_CMD_PIN      GPIO_NUM_40
#define SD_MMC_CLK_PIN      GPIO_NUM_38
#define SD_MMC_D0_PIN       GPIO_NUM_39
#define SD_MMC_D1_PIN       GPIO_NUM_41
#define SD_MMC_D2_PIN       GPIO_NUM_48
#define SD_MMC_D3_PIN       GPIO_NUM_47

// Battery
#define BATTERY_ADC_PIN     GPIO_NUM_9

// RGB LED
#define RGB_LED_PIN         GPIO_NUM_42

#endif // _BOARD_CONFIG_H_