#ifndef LCD_INIT_H
#define LCD_INIT_H
#ifdef LCD_IMPLEMENTATION
#include "lcd_config.h"
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
// extra drivers:
#include "esp_lcd_panel_ili9341.h"
#include "esp_lcd_panel_ili9342.h"
#include "esp_lcd_panel_ili9488.h"
#endif // LCD_IMPLEMENTATION
#include "esp_lcd_panel_io.h"

// global so it can be used after init
void lcd_panel_init(size_t max_transfer_size,esp_lcd_panel_io_color_trans_done_cb_t done_callback);
#if !defined(LCD_IMPLEMENTATION)
extern esp_lcd_panel_handle_t lcd_handle;
extern int lcd_width;
extern int lcd_height;
#else
esp_lcd_panel_handle_t lcd_handle;
#ifdef LCD_SWAP_XY
int lcd_width = LCD_VRES; // swapped
int lcd_height = LCD_HRES;
#else
int lcd_width = LCD_HRES;
int lcd_height = LCD_VRES;
#endif // LCD_SWAP_XY

// initialize the screen using the esp lcd panel API
void lcd_panel_init(size_t max_transfer_size,esp_lcd_panel_io_color_trans_done_cb_t done_callback) {
#ifdef PIN_NUM_BCKL
    gpio_set_direction((gpio_num_t)PIN_NUM_BCKL,GPIO_MODE_OUTPUT);
#endif // PIN_NUM_BCKL
#ifdef LCD_SPI_HOST // 1-bit SPI
    spi_bus_config_t bus_config;
    memset(&bus_config, 0, sizeof(bus_config));
    bus_config.sclk_io_num = PIN_NUM_CLK;
    bus_config.mosi_io_num = PIN_NUM_MOSI;
#ifdef PIN_NUM_MISO
    bus_config.miso_io_num = PIN_NUM_MISO;
#else
    bus_config.miso_io_num = -1;
#endif // PIN_NUM_MISO
#ifdef PIN_NUM_QUADWP
    bus_config.quadwp_io_num = PIN_NUM_QUADWP;
#else
    bus_config.quadwp_io_num = -1;
#endif
#ifdef PIN_NUM_QUADHD
    bus_config.quadhd_io_num = PIN_NUM_QUADHD;
#else
    bus_config.quadhd_io_num = -1;
#endif
    bus_config.max_transfer_sz = max_transfer_size + 8;

    // Initialize the SPI bus on LCD_SPI_HOST
    spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config;
    memset(&io_config, 0, sizeof(io_config));
    io_config.dc_gpio_num = PIN_NUM_DC,
    io_config.cs_gpio_num = PIN_NUM_CS,
    io_config.pclk_hz = LCD_PIXEL_CLOCK_HZ,
    io_config.lcd_cmd_bits = 8,
    io_config.lcd_param_bits = 8,
    io_config.spi_mode = 0,
    io_config.trans_queue_depth = 10,
    io_config.on_color_trans_done = done_callback;
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, 
                            &io_config, &
                            io_handle);
#elif defined(PIN_NUM_D07) // 8 or 16-bit i8080
    gpio_set_direction((gpio_num_t)PIN_NUM_RD,GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)PIN_NUM_RD,1);
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config;
    memset(&bus_config,0,sizeof(bus_config));
    bus_config.clk_src = LCD_CLK_SRC_PLL160M;
    bus_config.dc_gpio_num = PIN_NUM_RS;
    bus_config.wr_gpio_num = PIN_NUM_WR;
    bus_config.data_gpio_nums[0] = PIN_NUM_D00;
    bus_config.data_gpio_nums[1] = PIN_NUM_D01;
    bus_config.data_gpio_nums[2] = PIN_NUM_D02;
    bus_config.data_gpio_nums[3] = PIN_NUM_D03;
    bus_config.data_gpio_nums[4] = PIN_NUM_D04;
    bus_config.data_gpio_nums[5] = PIN_NUM_D05;
    bus_config.data_gpio_nums[6] = PIN_NUM_D06;
    bus_config.data_gpio_nums[7] = PIN_NUM_D07;
#ifdef PIN_NUM_D15
    bus_config.data_gpio_nums[8] = PIN_NUM_D08;
    bus_config.data_gpio_nums[9] = PIN_NUM_D09;
    bus_config.data_gpio_nums[10] = PIN_NUM_D10;
    bus_config.data_gpio_nums[11] = PIN_NUM_D11;
    bus_config.data_gpio_nums[12] = PIN_NUM_D12;
    bus_config.data_gpio_nums[13] = PIN_NUM_D13;
    bus_config.data_gpio_nums[14] = PIN_NUM_D14;
    bus_config.data_gpio_nums[15] = PIN_NUM_D15;
    bus_config.bus_width = 16;
#else
    bus_config.bus_width = 8;
#endif // PIN_NUM_D15
    bus_config.max_transfer_bytes = max_transfer_size;

    esp_lcd_new_i80_bus(&bus_config, &i80_bus);

    esp_lcd_panel_io_handle_t io_handle = NULL;

    esp_lcd_panel_io_i80_config_t io_config;
    memset(&io_config,0,sizeof(io_config));
    io_config.cs_gpio_num = PIN_NUM_CS;
    io_config.pclk_hz = LCD_PIXEL_CLOCK_HZ;
    io_config.trans_queue_depth = 20;
    io_config.dc_levels.dc_idle_level=0;
    io_config.dc_levels.dc_idle_level = 0;
    io_config.dc_levels.dc_cmd_level = 0;
    io_config.dc_levels.dc_dummy_level = 0;
    io_config.dc_levels.dc_data_level = 1;    
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.on_color_trans_done = done_callback;
    io_config.user_ctx = nullptr;
#ifdef LCD_SWAP_COLOR_BYTES
    io_config.flags.swap_color_bytes = LCD_SWAP_COLOR_BYTES;
#else
    io_config.flags.swap_color_bytes = false;
#endif // LCD_SWAP_COLOR_BYTES
    io_config.flags.cs_active_high = false;
    io_config.flags.reverse_color_bits = false;
    esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle);
#endif // PIN_NUM_D15
    lcd_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config;
    memset(&panel_config, 0, sizeof(panel_config));
#ifdef PIN_NUM_RST
    panel_config.reset_gpio_num = PIN_NUM_RST;
#else
    panel_config.reset_gpio_num = -1;
#endif
    panel_config.color_space = LCD_COLOR_SPACE;
    panel_config.bits_per_pixel = 16;

    // Initialize the LCD configuration
    LCD_PANEL(io_handle, &panel_config, &lcd_handle);
    
#ifdef PIN_NUM_BCKL
    // Turn off backlight to avoid unpredictable display on 
    // the LCD screen while initializing
    // the LCD panel driver. (Different LCD screens may need different levels)
    gpio_set_level((gpio_num_t)PIN_NUM_BCKL,LCD_BCKL_OFF_LEVEL);
#endif // PIN_NUM_BCKL
    // Reset the display
    esp_lcd_panel_reset(lcd_handle);

    // Initialize LCD panel
    esp_lcd_panel_init(lcd_handle);

    esp_lcd_panel_swap_xy(lcd_handle, LCD_SWAP_XY);
    esp_lcd_panel_set_gap(lcd_handle, LCD_GAP_X, LCD_GAP_Y);
    esp_lcd_panel_mirror(lcd_handle, LCD_MIRROR_X, LCD_MIRROR_Y);
    esp_lcd_panel_invert_color(lcd_handle, LCD_INVERT_COLOR);
    // Turn on the screen
    esp_lcd_panel_disp_off(lcd_handle, false);
#ifdef PIN_NUM_BCKL
    // Turn on backlight (Different LCD screens may need different levels)
    gpio_set_level((gpio_num_t)PIN_NUM_BCKL,LCD_BCKL_ON_LEVEL);
#endif // PIN_NUM_BCKL
}
#endif // LCD_IMPLEMENTATION
#endif // LCD_INIT_H