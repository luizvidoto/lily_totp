#pragma once

/* LCD CONFIG */
// Too low or too high pixel clock may cause screen mosaic
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (16 * 1000 * 1000)
// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 170
#define LVGL_LCD_BUF_SIZE (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)
#define EXAMPLE_PSRAM_DATA_ALIGNMENT 64

#define SCK_PIN     10 
#define MOSI_PIN    11 
#define MISO_PIN    12 
#define SDA_PIN     13 
#define RST_PIN     21

/*ESP32S3*/
#define PIN_LCD_BL 38

#define PIN_LCD_D0 39
#define PIN_LCD_D1 40
#define PIN_LCD_D2 41
#define PIN_LCD_D3 42
#define PIN_LCD_D4 45
#define PIN_LCD_D5 46
#define PIN_LCD_D6 47
#define PIN_LCD_D7 48

#define PIN_POWER_ON 15

#define PIN_LCD_RES 5
#define PIN_LCD_CS 6
#define PIN_LCD_DC 7
#define PIN_LCD_WR 8
#define PIN_LCD_RD 9

#define PIN_BUTTON_0 0
#define PIN_BUTTON_1 14
#define PIN_BAT_VOLT 4

#define PIN_IIC_SCL 17
#define PIN_IIC_SDA 18

#define PIN_TOUCH_INT 16
#define PIN_TOUCH_RES 21

// Display Pins
#define TFT_MOSI 35
#define TFT_SCLK 36
#define TFT_CS   34
#define TFT_DC   21
#define TFT_RST  47
#define TFT_BL   48

// Button Pins
#define BTN_PREV 3
#define BTN_NEXT 5
#define BTN_SEL  4

// Battery Monitor Pin
#define PIN_VBAT 1

// RFID Module Pins (if used)
#define RFID_SDA 7
#define RFID_SCL 15
#define RFID_RST 6
#define RFID_IRQ 16

