#include <Arduino.h>
// LCD_IMPLEMENTATION must be defined in a single
// source file, before lcd_init.h
#define LCD_IMPLEMENTATION
#include "lcd_init.h"
static uint8_t lcd_buffer[32*1024];
#ifndef LCD_PIN_NUM_HSYNC
static bool lcd_flushed = true;
static bool lcd_flush_ready(esp_lcd_panel_io_handle_t panel_io, 
                            esp_lcd_panel_io_event_data_t* edata, 
                            void* user_ctx) {
    lcd_flushed = true;
    return true;
}
#endif
void setup() {
  Serial.begin(115200);
#ifdef LCD_PIN_NUM_HSYNC
  lcd_panel_init();
#else
  lcd_panel_init(sizeof(lcd_buffer),lcd_flush_ready);
#endif
  memset(lcd_buffer,rand()&0xFF,sizeof(lcd_buffer));
  int width = lcd_width;
  if(width*2>sizeof(lcd_buffer)) {
    printf("Not enough transfer mem\n");
    while(1);
  }
  int height = sizeof(lcd_buffer)/(width*2);
  for(int y = 0;y<lcd_height;y+=height) {
    for(int x = 0;x<lcd_width;x+=width) {
      int x1 = x;
      int y1 = y;
      int x2 = x1 + width-1;
      int y2 = y1 + height-1;
#ifndef LCD_PIN_NUM_HSYNC
      while(!lcd_flushed) {
        vTaskDelay(1);
      }
      lcd_flushed = false;
#endif
      lcd_panel_draw_bitmap(x1,y1,x2,y2,lcd_buffer);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}