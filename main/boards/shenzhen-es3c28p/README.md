# Shenzhen ES3C28P Board Support

This directory contains board support for the Shenzhen ES3C28P development board based on ESP32-S3.

## Hardware Specifications

- **MCU**: ESP32-S3 (dual-core Xtensa LX7, 240 MHz)
- **Memory**: 512KB SRAM, 8MB PSRAM, 16MB Flash
- **Display**: 2.8" ILI9341V TFT LCD, 240x320 resolution, 16-bit RGB565
- **Touch**: FT6X36 capacitive touch controller, multi-touch
- **Audio**: ES8311 codec with I2S interface
- **Camera**: 8-bit DVP camera interface
- **Storage**: SDIO SD card interface
- **Connectivity**: WLAN 802.11 b/g/n, Bluetooth BLE and BT Classic
- **Power**: 5V/1A via Type-C connector
- **Dimensions**: 77.00 x 51.00 mm

## Pin Configuration

### Display (ILI9341V)
- MOSI: GPIO_11
- SCK: GPIO_12
- CS: GPIO_10
- DC: GPIO_46
- RST: Not connected (uses software reset)
- BL: GPIO_45

### Touch Controller (FT6X36)
- SDA: GPIO_16
- SCL: GPIO_15
- RST: GPIO_18
- INT: GPIO_17
- Address: 0x38

### Audio Codec (ES8311)
- DOUT: GPIO_6
- BCLK: GPIO_5
- WS: GPIO_7
- MCK: GPIO_4
- DIN: GPIO_8
- PA Enable: GPIO_1
- I2C SDA: GPIO_16
- I2C SCL: GPIO_15

### SD Card (SDIO)
- CMD: GPIO_40
- CLK: GPIO_38
- D0: GPIO_39
- D1: GPIO_41
- D2: GPIO_48
- D3: GPIO_47

### Battery
- ADC: GPIO_9

### RGB LED
- Control: GPIO_42

## Features

- ✅ 2.8" LCD display with touch support
- ✅ Audio input/output with ES8311 codec
- ✅ Camera interface support
- ✅ SD card storage
- ✅ Wi-Fi and Bluetooth connectivity
- ✅ USB Type-C power and programming

## Building

To build for this board:

```bash
python scripts/release.py shenzhen-es3c28p
```

Or manually:

```bash
idf.py set-target esp32s3
idf.py menuconfig
# Select "Shenzhen ES3C28P" under "Board Type"
idf.py build
idf.py flash monitor
```

## Notes

- The ILI9341V display controller uses standard ST7789 driver (compatible)
- Display resolution is 240x320 (width x height) for correct orientation
- FT6X36 touch controller is compatible with FT5x06 driver
- Board uses intouch display technology with special calibration requirements
- Camera interface supports 8-bit parallel data
- SD card uses SDIO interface for high-speed transfers

## Important Note

⚠️ **CORRECTED CONFIGURATION APPLIED** ⚠️

The original implementation had an incorrect display driver configuration that caused distorted images.
The display driver has been corrected to use ST7789 driver (compatible with ILI9341V) with proper orientation and color settings.