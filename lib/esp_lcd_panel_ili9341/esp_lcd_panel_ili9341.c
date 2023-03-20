/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "lcd_panel.ili9341";

// ili9341 commands
// All ILI9341 specific commands some are used by init()
#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0A
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR   0x30
#define ILI9341_VSCRDEF 0x33
#define ILI9341_MADCTL  0x36
#define ILI9341_VSCRSADD 0x37
#define ILI9341_PIXFMT  0x3A

#define ILI9341_WRDISBV  0x51
#define ILI9341_RDDISBV  0x52
#define ILI9341_WRCTRLD  0x53

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID4   0xD3
#define ILI9341_RDINDEX 0xD9
#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDIDX   0xDD // TBC

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

static esp_err_t panel_ili9341_del(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9341_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9341_init(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9341_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_ili9341_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_ili9341_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_ili9341_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_ili9341_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_ili9341_disp_on_off(esp_lcd_panel_t *panel, bool off);

typedef struct
{
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    unsigned int bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_cal; // save surrent value of LCD_CMD_COLMOD register
} ili9341_panel_t;

esp_err_t esp_lcd_new_panel_ili9341(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    ili9341_panel_t *ili9341 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    ili9341 = calloc(1, sizeof(ili9341_panel_t));
    ESP_GOTO_ON_FALSE(ili9341, ESP_ERR_NO_MEM, err, TAG, "no mem for ili9341 panel");

    if (panel_dev_config->reset_gpio_num >= 0)
    {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->color_space)
    {
    case ESP_LCD_COLOR_SPACE_RGB:
        ili9341->madctl_val = 0;
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        ili9341->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    switch (panel_dev_config->bits_per_pixel)
    {
    case 16:
        ili9341->colmod_cal = 0x55;
        break;
    case 18:
        ili9341->colmod_cal = 0x66;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    ili9341->io = io;
    ili9341->bits_per_pixel = panel_dev_config->bits_per_pixel;
    ili9341->reset_gpio_num = panel_dev_config->reset_gpio_num;
    ili9341->reset_level = panel_dev_config->flags.reset_active_high;
    ili9341->base.del = panel_ili9341_del;
    ili9341->base.reset = panel_ili9341_reset;
    ili9341->base.init = panel_ili9341_init;
    ili9341->base.draw_bitmap = panel_ili9341_draw_bitmap;
    ili9341->base.invert_color = panel_ili9341_invert_color;
    ili9341->base.set_gap = panel_ili9341_set_gap;
    ili9341->base.mirror = panel_ili9341_mirror;
    ili9341->base.swap_xy = panel_ili9341_swap_xy;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ili9341->base.disp_on_off = panel_ili9341_disp_on_off;
#else
    ili9341->base.disp_off = panel_ili9341_disp_on_off;
#endif
    *ret_panel = &(ili9341->base);
    ESP_LOGD(TAG, "new ili9341 panel @%p", ili9341);

    return ESP_OK;

err:
    if (ili9341)
    {
        if (panel_dev_config->reset_gpio_num >= 0)
        {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(ili9341);
    }
    return ret;
}

static esp_err_t panel_ili9341_del(esp_lcd_panel_t *panel)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    if (ili9341->reset_gpio_num >= 0)
    {
        gpio_reset_pin(ili9341->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del ili9341 panel @%p", ili9341);
    free(ili9341);
    return ESP_OK;
}

static esp_err_t panel_ili9341_reset(esp_lcd_panel_t *panel)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;

    // perform hardware reset
    if (ili9341->reset_gpio_num >= 0)
    {
        gpio_set_level(ili9341->reset_gpio_num, ili9341->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(ili9341->reset_gpio_num, !ili9341->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else
    {
        // perform software reset
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET << 8, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    return ESP_OK;
}

static esp_err_t panel_ili9341_init(esp_lcd_panel_t *panel)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    // vTaskDelay(pdMS_TO_TICKS(100));
    // esp_lcd_panel_io_tx_param(io, ILI9341_CMD_POSITIVE_GAMMA_CORRECTION,
    //                           (uint8_t[]){0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F}, 30);
    // esp_lcd_panel_io_tx_param(io, ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION,
    //                           (uint8_t[]){0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F}, 30);
    
    static const uint8_t params1[] = {0x03,0x80,02};
    esp_lcd_panel_io_tx_param(io,0xEF,params1,sizeof(params1));
    static const uint8_t params2[] = {0x00,0xc1,0x30};
    esp_lcd_panel_io_tx_param(io,0xCF,params2,sizeof(params2));
    static const uint8_t params3[] = {0x64,0x03,0x12,0x81};
    esp_lcd_panel_io_tx_param(io,0xED,params3,sizeof(params3));
    static const uint8_t params4[] = {0x85,0x00,0x78};
    esp_lcd_panel_io_tx_param(io,0xE8,params4,sizeof(params4));
    static const uint8_t params5[] = {0x39,0x2C,0x00,0x34,0x02};
    esp_lcd_panel_io_tx_param(io,0xCB,params5,sizeof(params5));
    static const uint8_t params6[] = {0x20}; 
    esp_lcd_panel_io_tx_param(io,0xF7,params6,sizeof(params6));
    static const uint8_t params7[] = { 0x00, 0x00 };
    esp_lcd_panel_io_tx_param(io,0xEA,params7,sizeof(params7));
    static const uint8_t params8[] = { 0x23 };
    esp_lcd_panel_io_tx_param(io,ILI9341_PWCTR1,params8,sizeof(params8));
    static const uint8_t params9[] = { 0x10 };
    esp_lcd_panel_io_tx_param(io,ILI9341_PWCTR2,params9,sizeof(params9));
    static const uint8_t params10[] = { 0x3e, 0x28 };
    esp_lcd_panel_io_tx_param(io,ILI9341_VMCTR1,params10,sizeof(params10));
    static const uint8_t params11[] = { 0x86 };
    esp_lcd_panel_io_tx_param(io,ILI9341_VMCTR2,params11,sizeof(params11));
    static const uint8_t params12[] = { 0x40 | 0x08 };
    esp_lcd_panel_io_tx_param(io,ILI9341_MADCTL,params12,sizeof(params12));
    static const uint8_t params13[] = {0x55};
    esp_lcd_panel_io_tx_param(io,ILI9341_PIXFMT,params13,sizeof(params13));
    static const uint8_t params14[] = {0x00,0x13};
    esp_lcd_panel_io_tx_param(io,ILI9341_FRMCTR1,params14,sizeof(params14));
    static const uint8_t params15[] = {0x08,0x82,0x27};
    esp_lcd_panel_io_tx_param(io,ILI9341_DFUNCTR,params15,sizeof(params15));
    static const uint8_t params16[] = {0x00};
    esp_lcd_panel_io_tx_param(io,0xF2,params16,sizeof(params16));
    static const uint8_t params17[] = {0x01};
    esp_lcd_panel_io_tx_param(io,ILI9341_GAMMASET,params17,sizeof(params17));
    static const uint8_t params18[] = {0x0f,0x31,0x2b,0x0c,0x0e,0x08,0x4e,0xf1,0x37,0x07,0x10,0x03,0x0e,0x09,0x00};
    esp_lcd_panel_io_tx_param(io,ILI9341_GMCTRP1,params18,sizeof(params18));
    static const uint8_t params19[] = {0x00,0x0e,0x14,0x03,0x11,0x07,0x31,0xc1,0x48,0x08,0x0f,0x0c,0x31,0x36,0x0f};
    esp_lcd_panel_io_tx_param(io,ILI9341_GMCTRN1,params19,sizeof(params19));
    static const uint8_t params20[] = {};
    esp_lcd_panel_io_tx_param(io,ILI9341_SLPOUT,params20,0);
    vTaskDelay(pdMS_TO_TICKS(120));
    return ESP_OK;
}

static esp_err_t panel_ili9341_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    //assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = ili9341->io;
    x_start += ili9341->x_gap;
    x_end += ili9341->x_gap;
    y_start += ili9341->y_gap;
    y_end += ili9341->y_gap;
    // define an area of frame memory where MCU can access
    uint8_t data1[4];
    data1[0]=(x_start>>8)&0xFF;
    data1[1]=x_start&0xFF;
    data1[2]=((x_end-1)>>8)&0xFF;
    data1[3]=(x_end-1)&0xFF;
    esp_lcd_panel_io_tx_param(io, 0x2A, data1, 4);
    uint8_t data2[4];
    data2[0]=(y_start>>8)&0xFF;
    data2[1]=y_start&0xFF;
    data2[2]=((y_end-1)>>8)&0xFF;
    data2[3]=(y_end-1)&0xFF;
    esp_lcd_panel_io_tx_param(io, 0x2B, data2, 4);
    
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * ili9341->bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, 0x2C, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_ili9341_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    int command = 0;
    if (invert_color_data)
    {
        command = LCD_CMD_INVON;
    }
    else
    {
        command = LCD_CMD_INVOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}

static esp_err_t panel_ili9341_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    if (mirror_x)
    {
        ili9341->madctl_val |= LCD_CMD_MX_BIT;
    }
    else
    {
        ili9341->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y)
    {
        ili9341->madctl_val |= LCD_CMD_MY_BIT;
    }
    else
    {
        ili9341->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL , &ili9341->madctl_val, 1);

    return ESP_OK;
}

static esp_err_t panel_ili9341_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    if (swap_axes)
    {
        ili9341->madctl_val |= LCD_CMD_MV_BIT;
    }
    else
    {
        ili9341->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &ili9341->madctl_val, 1);

    return ESP_OK;
}

static esp_err_t panel_ili9341_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    ili9341->x_gap = x_gap;
    ili9341->y_gap = y_gap;

    return ESP_OK;
}

static esp_err_t panel_ili9341_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    ili9341_panel_t *ili9341 = __containerof(panel, ili9341_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9341->io;
    int command = 0;
    if (!on_off)
    {
        command = LCD_CMD_DISPON;
    }
    else
    {
        command = LCD_CMD_DISPOFF;
    }
    esp_lcd_panel_io_tx_param(io, command , NULL, 0);
    // SEG/COM will be ON/OFF after 100ms after sending DISP_ON/OFF command
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}
