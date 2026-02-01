#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_adc/adc_oneshot.h"
// #include "esp_lcd_st7262.h"  // Not using ST7262, using ILI9341V instead
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_system.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <driver/spi_common.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_pm.h>

#define TAG "ShenzhenEs3c28pBoard"

class ShenzhenEs3c28pBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_;
    LcdDisplay* display_;
    esp_lcd_touch_handle_t touch_handle_ = nullptr;
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;

    // I2C initialization
    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = ESP32_I2C_HOST,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    // SPI initialization for display
    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        
        ESP_ERROR_CHECK(spi_bus_initialize(ESP32_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // Initialize ILI9341V display controller
    void InitializeDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // Configure SPI for display
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = 0;  // ILI9341V uses mode 0
        io_config.pclk_hz = 16 * 1000 * 1000;  // 16MHz clock for stability with ILI9341V
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(ESP32_LCD_HOST, &io_config, &panel_io));

        // Configure display panel
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;  // ILI9341V actually uses BGR order
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        // Initialize the display panel
        esp_lcd_panel_reset(panel);
        vTaskDelay(pdMS_TO_TICKS(100));  // Add delay after reset
        esp_lcd_panel_init(panel);
        vTaskDelay(pdMS_TO_TICKS(100));  // Add delay after init
        esp_lcd_panel_invert_color(panel, true);  // ILI9341V needs inverted colors for correct display
        esp_lcd_panel_swap_xy(panel, false);  // Try without XY swap
        esp_lcd_panel_mirror(panel, false, true);  // Try only Y mirror

        // Set display brightness via PWM
        esp_lcd_panel_disp_on_off(panel, true);
        vTaskDelay(pdMS_TO_TICKS(50));  // Add delay after turning on display

        // Create display object with corrected orientation
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                    DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                    false, true, false);  // Try only Y mirror
    }

    // Initialize FT6X36 touch controller (using FT5X06 driver as they are compatible)
    void InitializeTouch() {
        const esp_lcd_touch_config_t tp_cfg = {
            .x_max = DISPLAY_WIDTH,
            .y_max = DISPLAY_HEIGHT,
            .rst_gpio_num = TOUCH_RST_PIN,
            .int_gpio_num = TOUCH_INT_PIN,
            .levels = {
                .reset = 0,
                .interrupt = 0,
            },
            .flags = {
                .swap_xy = 0,
                .mirror_x = 0,
                .mirror_y = 0,
            },
        };

        esp_lcd_panel_io_handle_t tp_io_handle = nullptr;
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
        tp_io_config.dev_addr = TOUCH_I2C_ADDR;
        tp_io_config.scl_speed_hz = 400000;

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(codec_i2c_bus_, &tp_io_config, &tp_io_handle));
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &touch_handle_));
    }

    // Initialize buttons
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    // Initialize ADC for battery monitoring
    void InitializeBattery() {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle_));

        // Configure the specific ADC channel for battery monitoring
        // GPIO_NUM_9 maps to ADC1 channel 0 on ESP32-S3
        adc_oneshot_chan_cfg_t config = {};
        config.bitwidth = ADC_BITWIDTH_DEFAULT;
        config.atten = ADC_ATTEN_DB_12;  // For wider input voltage range
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, ADC_CHANNEL_0, &config));
    }

    // Initialize RGB LED
    void InitializeRgbLed() {
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_13_BIT,
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 5000,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

        ledc_channel_config_t ledc_channel = {
            .gpio_num = RGB_LED_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    // MCP Tools Initialization
    void InitializeTools() {
        // Add MCP tools here if needed
        // Example: register speaker, screen, battery, light controls
    }

public:
    ShenzhenEs3c28pBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeSpi();
        InitializeDisplay();
        InitializeTouch();
        InitializeButtons();
        InitializeBattery();
        InitializeRgbLed();
        InitializeTools();
        GetBacklight()->SetBrightness(100);
    }

    // Get audio codec
    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(
            codec_i2c_bus_,
            ESP32_I2C_HOST,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR,
            true,   // use_mclk
            true);  // pa_inverted = true (active low amplifier)
        return &audio_codec;
    }

    // Get display
    virtual Display* GetDisplay() override {
        return display_;
    }

    // Get backlight control
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    // Get touch controller
    esp_lcd_touch_handle_t GetTouchHandle() {
        return touch_handle_;
    }

    // Get battery level - matching parent class signature
    bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        if (!adc_handle_) {
            level = -1;
            charging = false;
            discharging = false;
            return false;
        }

        int raw_reading = 0;
        // Use ADC channel 0 which corresponds to GPIO_NUM_9 on ESP32-S3
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, ADC_CHANNEL_0, &raw_reading));

        // Convert raw ADC reading to voltage (assuming 3.3V reference)
        float voltage = (raw_reading * 3.3f) / 4095.0f;

        // Simple linear approximation for battery level (adjust as needed)
        // Assuming 3.0V is empty and 4.2V is full
        level = (int)((voltage - 3.0f) / (4.2f - 3.0f) * 100.0f);
        if (level > 100) level = 100;
        if (level < 0) level = 0;

        // For now, assume discharging state - can be improved with more sophisticated logic
        charging = false;
        discharging = true;

        return true;
    }

    // Set RGB LED color - currently only using one channel, can be expanded for full RGB
    void SetRgbLedColor(uint8_t red, uint8_t green, uint8_t blue) {
        // Simple implementation - for now just average the RGB values
        // To implement full RGB, you would need to configure multiple LEDC channels
        uint8_t avg_brightness = (red + green + blue) / 3;
        uint32_t duty = (avg_brightness * 8191) / 255;    // 13-bit resolution

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
};

// Register the board
DECLARE_BOARD(ShenzhenEs3c28pBoard);