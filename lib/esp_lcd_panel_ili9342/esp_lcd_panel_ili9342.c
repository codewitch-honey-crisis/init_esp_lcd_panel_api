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

static const char *TAG = "lcd_panel.ili9342";

// ili9342 commands
// All ILI9342 specific commands some are used by init()
#define ILI9342_NOP     0x00
#define ILI9342_SWRESET 0x01
#define ILI9342_RDDID   0x04
#define ILI9342_RDDST   0x09

#define ILI9342_SLPIN   0x10
#define ILI9342_SLPOUT  0x11
#define ILI9342_PTLON   0x12
#define ILI9342_NORON   0x13

#define ILI9342_RDMODE  0x0A
#define ILI9342_RDMADCTL  0x0B
#define ILI9342_RDPIXFMT  0x0C
#define ILI9342_RDIMGFMT  0x0A
#define ILI9342_RDSELFDIAG  0x0F

#define ILI9342_INVOFF  0x20
#define ILI9342_INVON   0x21
#define ILI9342_GAMMASET 0x26
#define ILI9342_DISPOFF 0x28
#define ILI9342_DISPON  0x29

#define ILI9342_CASET   0x2A
#define ILI9342_PASET   0x2B
#define ILI9342_RAMWR   0x2C
#define ILI9342_RAMRD   0x2E

#define ILI9342_PTLAR   0x30
#define ILI9342_VSCRDEF 0x33
#define ILI9342_MADCTL  0x36
#define ILI9342_VSCRSADD 0x37
#define ILI9342_PIXFMT  0x3A

#define ILI9342_WRDISBV  0x51
#define ILI9342_RDDISBV  0x52
#define ILI9342_WRCTRLD  0x53

#define ILI9342_FRMCTR1 0xB1
#define ILI9342_FRMCTR2 0xB2
#define ILI9342_FRMCTR3 0xB3
#define ILI9342_INVCTR  0xB4
#define ILI9342_DFUNCTR 0xB6

#define ILI9342_PWCTR1  0xC0
#define ILI9342_PWCTR2  0xC1
#define ILI9342_PWCTR3  0xC2
#define ILI9342_PWCTR4  0xC3
#define ILI9342_PWCTR5  0xC4
#define ILI9342_VMCTR1  0xC5
#define ILI9342_VMCTR2  0xC7

#define ILI9342_RDID4   0xD3
#define ILI9342_RDINDEX 0xD9
#define ILI9342_RDID1   0xDA
#define ILI9342_RDID2   0xDB
#define ILI9342_RDID3   0xDC
#define ILI9342_RDIDX   0xDD // TBC

#define ILI9342_GMCTRP1 0xE0
#define ILI9342_GMCTRN1 0xE1

#define ILI9342_MADCTL_MY  0x80
#define ILI9342_MADCTL_MX  0x40
#define ILI9342_MADCTL_MV  0x20
#define ILI9342_MADCTL_ML  0x10
#define ILI9342_MADCTL_RGB 0x00
#define ILI9342_MADCTL_BGR 0x08
#define ILI9342_MADCTL_MH  0x04

static esp_err_t panel_ili9342_del(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9342_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9342_init(esp_lcd_panel_t *panel);
static esp_err_t panel_ili9342_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_ili9342_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_ili9342_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_ili9342_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_ili9342_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_ili9342_disp_on_off(esp_lcd_panel_t *panel, bool off);

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
} ili9342_panel_t;

esp_err_t esp_lcd_new_panel_ili9342(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    ili9342_panel_t *ili9342 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    ili9342 = calloc(1, sizeof(ili9342_panel_t));
    ESP_GOTO_ON_FALSE(ili9342, ESP_ERR_NO_MEM, err, TAG, "no mem for ili9342 panel");

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
        ili9342->madctl_val = 0;
        break;
    case ESP_LCD_COLOR_SPACE_BGR:
        ili9342->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    switch (panel_dev_config->bits_per_pixel)
    {
    case 16:
        ili9342->colmod_cal = 0x55;
        break;
    case 18:
        ili9342->colmod_cal = 0x66;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    ili9342->io = io;
    ili9342->bits_per_pixel = panel_dev_config->bits_per_pixel;
    ili9342->reset_gpio_num = panel_dev_config->reset_gpio_num;
    ili9342->reset_level = panel_dev_config->flags.reset_active_high;
    ili9342->base.del = panel_ili9342_del;
    ili9342->base.reset = panel_ili9342_reset;
    ili9342->base.init = panel_ili9342_init;
    ili9342->base.draw_bitmap = panel_ili9342_draw_bitmap;
    ili9342->base.invert_color = panel_ili9342_invert_color;
    ili9342->base.set_gap = panel_ili9342_set_gap;
    ili9342->base.mirror = panel_ili9342_mirror;
    ili9342->base.swap_xy = panel_ili9342_swap_xy;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ili9342->base.disp_on_off = panel_ili9342_disp_on_off;
#else
    ili9342->base.disp_off = panel_ili9342_disp_on_off;
#endif
    *ret_panel = &(ili9342->base);
    ESP_LOGD(TAG, "new ili9342 panel @%p", ili9342);

    return ESP_OK;

err:
    if (ili9342)
    {
        if (panel_dev_config->reset_gpio_num >= 0)
        {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(ili9342);
    }
    return ret;
}

static esp_err_t panel_ili9342_del(esp_lcd_panel_t *panel)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    if (ili9342->reset_gpio_num >= 0)
    {
        gpio_reset_pin(ili9342->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del ili9342 panel @%p", ili9342);
    free(ili9342);
    return ESP_OK;
}

static esp_err_t panel_ili9342_reset(esp_lcd_panel_t *panel)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9342->io;

    // perform hardware reset
    if (ili9342->reset_gpio_num >= 0)
    {
        gpio_set_level(ili9342->reset_gpio_num, ili9342->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(ili9342->reset_gpio_num, !ili9342->reset_level);
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

static esp_err_t panel_ili9342_init(esp_lcd_panel_t *panel)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9342->io;
    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    // vTaskDelay(pdMS_TO_TICKS(100));
    // esp_lcd_panel_io_tx_param(io, ILI9342_CMD_POSITIVE_GAMMA_CORRECTION,
    //                           (uint8_t[]){0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F}, 30);
    // esp_lcd_panel_io_tx_param(io, ILI9342_CMD_NEGATIVE_GAMMA_CORRECTION,
    //                           (uint8_t[]){0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F}, 30);
    
    static const uint8_t params1[] = {0xFF,0x93,0x42};
    esp_lcd_panel_io_tx_param(io,0xC8,params1,sizeof(params1));
    static const uint8_t params2[] = {0x12,0x12};
    esp_lcd_panel_io_tx_param(io,0xC0,params2,sizeof(params2));
    static const uint8_t params3[] = {0x03};
    esp_lcd_panel_io_tx_param(io,0xC1,params3,sizeof(params3));
    static const uint8_t params4[] = {0x0E};
    esp_lcd_panel_io_tx_param(io,0xB0,params4,sizeof(params4));
    static const uint8_t params5[] = {0x00,0x01,0x01};
    esp_lcd_panel_io_tx_param(io,0xF6,params5,sizeof(params5));
    static const uint8_t params6[] = {0x80|0x20|0x08}; 
    esp_lcd_panel_io_tx_param(io,0x36,params6,sizeof(params6));
    esp_lcd_panel_io_tx_param(io,0x3A,&ili9342->colmod_cal ,1);
    static const uint8_t params7[] = { 0x08,0x82,0x27 };
    esp_lcd_panel_io_tx_param(io,0xB6,params7,sizeof(params7));
    static const uint8_t params8[] = { 0x00,0x0C,0x11,0x04,0x11,0x08,0x37,0x89,0x4C,0x06,0x0C,0x0A,0x2E,0x34,0x0F };
    esp_lcd_panel_io_tx_param(io,0xE0,params8,sizeof(params8));
    static const uint8_t params9[] = { 0x00,0x0B,0x11,0x05,0x13,0x09,0x33,0x67,0x48,0x07,0x0E,0x0B,0x2E,0x33,0x0F };
    esp_lcd_panel_io_tx_param(io,0xE1,params9,sizeof(params9));
    static const uint8_t params10[] = {};
    esp_lcd_panel_io_tx_param(io,0x11,params10,0);
    vTaskDelay(pdMS_TO_TICKS(120));
    return ESP_OK;
}

static esp_err_t panel_ili9342_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    //assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = ili9342->io;
    x_start += ili9342->x_gap;
    x_end += ili9342->x_gap;
    y_start += ili9342->y_gap;
    y_end += ili9342->y_gap;
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
    size_t len = (x_end - x_start) * (y_end - y_start) * ili9342->bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, 0x2C, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_ili9342_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9342->io;
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

static esp_err_t panel_ili9342_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9342->io;
    if (mirror_x)
    {
        ili9342->madctl_val |= LCD_CMD_MX_BIT;
    }
    else
    {
        ili9342->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y)
    {
        ili9342->madctl_val |= LCD_CMD_MY_BIT;
    }
    else
    {
        ili9342->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL , &ili9342->madctl_val, 1);

    return ESP_OK;
}

static esp_err_t panel_ili9342_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9342->io;
    if (swap_axes)
    {
        ili9342->madctl_val |= ~LCD_CMD_MV_BIT;
    }
    else
    {
        ili9342->madctl_val &= LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &ili9342->madctl_val, 1);

    return ESP_OK;
}

static esp_err_t panel_ili9342_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    ili9342->x_gap = x_gap;
    ili9342->y_gap = y_gap;

    return ESP_OK;
}

static esp_err_t panel_ili9342_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    ili9342_panel_t *ili9342 = __containerof(panel, ili9342_panel_t, base);
    esp_lcd_panel_io_handle_t io = ili9342->io;
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
