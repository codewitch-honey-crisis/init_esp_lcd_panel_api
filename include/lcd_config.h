#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#ifdef TTGO_T1
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 19
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 5
#define LCD_PIN_NUM_DC 16
#define LCD_PIN_NUM_RST 23
#define LCD_PIN_NUM_BCKL 4
#define LCD_PANEL esp_lcd_new_panel_st7789
#define LCD_HRES 135
#define LCD_VRES 240
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_RGB
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 40
#define LCD_GAP_Y 52
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y true
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#define LCD_TRANSFER_SIZE (32*1024)
#define LCD_FLUSH_CALLBACK lcd_flush_ready
#endif // TTGO_T1

#ifdef ESP_WROVER_KIT
#define LCD_BCKL_ON_LEVEL 0
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_SPI_HOST    HSPI_HOST
#define LCD_PIN_NUM_MISO 25
#define LCD_PIN_NUM_MOSI 23
#define LCD_PIN_NUM_CLK  19
#define LCD_PIN_NUM_CS   22
#define LCD_PIN_NUM_DC   21
#define LCD_PIN_NUM_RST  18
#define LCD_PIN_NUM_BCKL 5
#define LCD_PANEL esp_lcd_new_panel_ili9341
#define LCD_HRES 240
#define LCD_VRES 320
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR false
#define LCD_SWAP_XY true
#endif // ESP_WROVER_KIT

#ifdef ESP_DISPLAY_S3
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_CS 37
#define LCD_PIN_NUM_WR 35
#define LCD_PIN_NUM_RD 48
#define LCD_PIN_NUM_RS 36
#define LCD_PIN_NUM_D00 47
#define LCD_PIN_NUM_D01 21
#define LCD_PIN_NUM_D02 14
#define LCD_PIN_NUM_D03 13
#define LCD_PIN_NUM_D04 12
#define LCD_PIN_NUM_D05 11
#define LCD_PIN_NUM_D06 10
#define LCD_PIN_NUM_D07 9
#define LCD_PIN_NUM_D08 3
#define LCD_PIN_NUM_D09 8
#define LCD_PIN_NUM_D10 16
#define LCD_PIN_NUM_D11 15
#define LCD_PIN_NUM_D12 7
#define LCD_PIN_NUM_D13 6
#define LCD_PIN_NUM_D14 5
#define LCD_PIN_NUM_D15 4
#define LCD_PIN_NUM_BCKL 45
#define LCD_PANEL esp_lcd_new_panel_ili9488
#define LCD_HRES 320
#define LCD_VRES 480
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR false
#define LCD_SWAP_XY true
#define LCD_SWAP_COLOR_BYTES true
#endif // ESP_DISPLAY_S3

#ifdef M5STACK_CORE2
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 23
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 5
#define LCD_PIN_NUM_DC 15
#define LCD_PANEL esp_lcd_new_panel_ili9342
#define LCD_HRES 240
#define LCD_VRES 320
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#endif // M5STACK_CORE2

#ifdef M5STACK_FIRE
#define LCD_SPI_HOST    SPI3_HOST
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_MOSI 23
#define LCD_PIN_NUM_CLK 18
#define LCD_PIN_NUM_CS 14
#define LCD_PIN_NUM_DC 27
#define LCD_PIN_NUM_RST 33
#define LCD_PIN_NUM_BCKL 32
#define LCD_PANEL esp_lcd_new_panel_ili9342
#define LCD_HRES 240
#define LCD_VRES 320
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 0
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y false
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#endif // M5STACK_FIRE

#ifdef ESP_DISPLAY_4INCH
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_CS 1
#define LCD_PIN_NUM_SCK 12
#define LCD_PIN_NUM_SDA 11 
#define LCD_PIN_NUM_DE 45
#define LCD_PIN_NUM_VSYNC 4
#define LCD_PIN_NUM_HSYNC 5
#define LCD_PIN_NUM_CLK 21
#define LCD_PIN_NUM_D00 6
#define LCD_PIN_NUM_D01 7
#define LCD_PIN_NUM_D02 15
#define LCD_PIN_NUM_D03 16
#define LCD_PIN_NUM_D04 8
#define LCD_PIN_NUM_D05 0
#define LCD_PIN_NUM_D06 9
#define LCD_PIN_NUM_D07 14
#define LCD_PIN_NUM_D08 47
#define LCD_PIN_NUM_D09 48
#define LCD_PIN_NUM_D10 3
#define LCD_PIN_NUM_D11 39
#define LCD_PIN_NUM_D12 40
#define LCD_PIN_NUM_D13 41
#define LCD_PIN_NUM_D14 42
#define LCD_PIN_NUM_D15 2
#define LCD_PIN_NUM_BCKL -1
#define LCD_HSYNC_POLARITY 0
#define LCD_HSYNC_FRONT_PORCH 10
#define LCD_HSYNC_PULSE_WIDTH 8
#define LCD_HSYNC_BACK_PORCH 50
#define LCD_VSYNC_POLARITY 0
#define LCD_VSYNC_FRONT_PORCH 10
#define LCD_VSYNC_PULSE_WIDTH 8
#define LCD_VSYNC_BACK_PORCH 20
#define LCD_CLK_IDLE_HIGH 0
#define LCD_DE_IDLE_HIGH 1
#define LCD_BIT_DEPTH 16
#define LCD_PANEL esp_lcd_new_panel_st7701
#define LCD_HRES 480
#define LCD_VRES 480
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_BGR
#define LCD_SWAP_COLOR_BYTES true
#ifdef CONFIG_SPIRAM_MODE_QUAD
    #define LCD_PIXEL_CLOCK_HZ (6 * 1000 * 1000)
#else
    #define LCD_PIXEL_CLOCK_HZ (14 * 1000 * 1000)
#endif
#endif // ESP_DISPLAY_4INCH

#ifdef T_DISPLAY_S3
#define LCD_BCKL_ON_LEVEL 1
#define LCD_BCKL_OFF_LEVEL !LCD_BCKL_ON_LEVEL
#define LCD_PIN_NUM_CS 6
#define LCD_PIN_NUM_RST 5
#define LCD_PIN_NUM_WR 8
#define LCD_PIN_NUM_RD 9
#define LCD_PIN_NUM_RS 7
#define LCD_PIN_NUM_D00 39
#define LCD_PIN_NUM_D01 40
#define LCD_PIN_NUM_D02 41
#define LCD_PIN_NUM_D03 42
#define LCD_PIN_NUM_D04 45
#define LCD_PIN_NUM_D05 46
#define LCD_PIN_NUM_D06 47
#define LCD_PIN_NUM_D07 48
#define LCD_PIN_NUM_BCKL 38
#define LCD_PANEL esp_lcd_new_panel_st7789
#define LCD_HRES 170
#define LCD_VRES 320
#define LCD_COLOR_SPACE ESP_LCD_COLOR_SPACE_RGB
#define LCD_PIXEL_CLOCK_HZ (6528 * 1000)
#define LCD_GAP_X 0
#define LCD_GAP_Y 35
#define LCD_MIRROR_X false
#define LCD_MIRROR_Y true
#define LCD_INVERT_COLOR true
#define LCD_SWAP_XY true
#endif // T_DISPLAY_S3

#endif // LCD_CONFIG_H