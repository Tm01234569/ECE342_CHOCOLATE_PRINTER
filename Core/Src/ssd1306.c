/**
* original author:  Tilen Majerle<tilen@majerle.eu>
* modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>
 ----------------------------------------------------------------------
        Copyright (C) Alexander Lutsai, 2016
  Copyright (C) Tilen Majerle, 2015
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ----------------------------------------------------------------------
*/
#include "ssd1306.h"
extern I2C_HandleTypeDef hi2c2;
/* SSD1306 data buffer. This is the buffer you must write to for setting pixel
 * values. */
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
/* The set of registers to write to when initializing the SSD1306 screen. */
static uint8_t SSD1306_Init_Config[] = {
    0xAE, 0x20, 0x10, 0xB0, 0xC8, 0x00, 0x10, 0x40, 0x81, 0xFF,
    0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5, 0xF0, 0xD9,
    0x22, 0xDA, 0x12, 0xDB, 0x20, 0x8D, 0x14, 0xAF, 0x2E};
/* An array you can use for the commands to be sent for scrolling. */
static uint8_t SSD1306_Scroll_Commands[8] = {0};
/* The following functions are provided for you to use, without requiring any
 * modifications */
void SSD1306_UpdateScreen(void) {
  uint8_t m;
  uint8_t packet[3] = {0x00, 0x00, 0x10};
  for (m = 0; m < 8; m++) {
    packet[0] = (0xB0 + m);
    ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, packet, 3);
    ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x40,
                      &SSD1306_Buffer[SSD1306_WIDTH * m], SSD1306_WIDTH);
  }
}
void SSD1306_Fill(SSD1306_COLOR_t color) {
  /* Use memset to efficiently set the entire SSD1306_Buffer to a single value.
   */
  memset(SSD1306_Buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF,
         sizeof(SSD1306_Buffer));
}
void SSD1306_Clear(void) {
  SSD1306_Fill(0);
  SSD1306_UpdateScreen();
}
/* Start of the functions you must complete for this lab. */
#define SSD1306_TXBUF_MAX (1 + SSD1306_WIDTH)  // 129 if width = 128
static uint8_t tx_buf[SSD1306_TXBUF_MAX];
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t* data,
                       uint16_t count) {
  /* TODO */
  // Safety: clamp count so we don't overflow tx_buf
  if (count > (SSD1306_TXBUF_MAX - 1)) {
    count = SSD1306_TXBUF_MAX - 1;
  }
  tx_buf[0] = reg;  // control byte
  // copy count bytes of payload
  for (uint16_t i = 0; i < count; i++) {
    tx_buf[i + 1] = data[i];
  }
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c2, address, tx_buf,
                                                     count + 1, HAL_MAX_DELAY);
  if (status == HAL_OK) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);    // Green LED on
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);  // Error LED off
  } else {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // Green LED off
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);    // Error LED on
  }
}
HAL_StatusTypeDef SSD1306_Init(void) {
  /* Check if the OLED is connected to I2C */
  if (HAL_I2C_IsDeviceReady(&hi2c2, SSD1306_I2C_ADDR, 1, 20000) != HAL_OK) {
    return HAL_ERROR;
  }
  /* Keep this delay to prevent overflowing the I2C controller */
  HAL_Delay(10);
  /* Init LCD */
  for (uint16_t i = 0; i < sizeof(SSD1306_Init_Config); i++) {
    uint8_t cmd = SSD1306_Init_Config[i];
    // control byte 0x00 = command path
    ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, &cmd, 1);
  }
  /* Clear screen */
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  /* Update screen */
  SSD1306_UpdateScreen();
  /* Return OK */
  return HAL_OK;
}
HAL_StatusTypeDef SSD1306_SetPixel(uint16_t x, uint16_t y,
                                   SSD1306_COLOR_t color) {
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
    return HAL_ERROR;
  }
  uint16_t page = y / 8;
  uint16_t index = x + (page * SSD1306_WIDTH);
  uint8_t bit = y % 8;
  /* Set the pixel at position (x,y) to 'color'. */
  if (color == SSD1306_COLOR_WHITE) {
    /* TODO */
    SSD1306_Buffer[index] |= (1 << bit);
  } else {
    SSD1306_Buffer[index] &= ~(1 << bit);
  }
  return HAL_OK;
}
void SSD1306_Scroll(SSD1306_SCROLL_DIR_t direction, uint8_t start_row,
                    uint8_t end_row) {
  /* TO DO */
  // Pages are 0..7 on a 64-pixel-tall SSD1306 (8 pages total)
  if (start_row > 7) start_row = 7;
  if (end_row > 7) end_row = 7;
  if (start_row > end_row) {
    uint8_t tmp = start_row;
    start_row = end_row;
    end_row = tmp;
  }
  // Optional but recommended: stop any existing scroll first
  //	    {
  //	        uint8_t stop = SSD1306_DEACTIVATE_SCROLL;
  //	        ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, &stop, 1);
  //	    }
  SSD1306_Scroll_Commands[0] = (direction == SSD1306_SCROLL_RIGHT)
                                   ? SSD1306_RIGHT_HORIZONTAL_SCROLL
                                   : SSD1306_LEFT_HORIZONTAL_SCROLL;
  SSD1306_Scroll_Commands[1] = 0x00;       // dummy
  SSD1306_Scroll_Commands[2] = start_row;  // first page
  SSD1306_Scroll_Commands[3] = 0x00;       // speed (default)
  SSD1306_Scroll_Commands[4] = end_row;    // last page
  SSD1306_Scroll_Commands[5] = 0x00;       // dummy
  SSD1306_Scroll_Commands[6] = 0xFF;       // dummy
  SSD1306_Scroll_Commands[7] = 0x2F;       // 0x2F
  // Send all 8 bytes as commands
  ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, SSD1306_Scroll_Commands, 8);
}
void SSD1306_Stopscroll(void) {
  /* TO DO */
  uint8_t cmd = 0x2E;  // Deactivate scroll
  ssd1306_I2C_Write(SSD1306_I2C_ADDR, 0x00, &cmd, 1);
}
void SSD1306_Putc(uint16_t x, uint16_t y, char ch, FontDef_t* Font) {
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
    return;  // This is lowk not necessary but making sure x and y are within
             // bounds of the screen
  // ASCII printable characters are from 32 to 126 so have to check if the
  // character is valid or not
  if (ch < 32 || ch > 126) {
    ch = '?';  // fallback because its an invalid character (So if its like an
               // invalid character it falls back to '?'
    if (ch < 32 || ch > 126) return;
  }
  // Each character = Font->FontHeight rows.
  // For Font11x18: height=18, and each row is a uint16_t.
  // Typical layout: data is packed as [char0 row0..rowH-1, char1 row0.., ...]
  const uint16_t* char_data =
      (const uint16_t*)&Font
          ->data[(ch - 32) * Font->FontHeight];  // We need to find the bitmap
                                                 // of this character
  for (uint16_t row = 0; row < Font->FontHeight;
       row++) {  // For 11x18 it goes from 0 to 18
    uint16_t bits = char_data[row];
    // Only the most significant Font->FontWidth bits are valid.
    // For Font11x18, width=11: check bits 15 down to 5.
    for (uint16_t col = 0; col < Font->FontWidth;
         col++) {  // Col = 0 is left most pixel and col = 10 is right most
                   // pixel for 11x18
      // MSB-first: col=0 tests bit15, col=10 tests bit(15-10)=5
      uint16_t mask = (uint16_t)(1U << (15 - col));
      if (bits & mask) {
        SSD1306_SetPixel(x + col, y + row, SSD1306_COLOR_WHITE);
      } else {
        // Optional: draw background as black
        SSD1306_SetPixel(x + col, y + row, SSD1306_COLOR_BLACK);
      }
    }
  }
}
HAL_StatusTypeDef SSD1306_Puts(char* str, FontDef_t* Font) {
  // Static cursor survives between calls
  static uint16_t cx = 0;
  static uint16_t cy = 0;
  while (*str != '\0') {
    // Handle newline
    if (*str == '\n') {  // For new line
      cx = 0;
      cy += Font->FontHeight;
      str++;
      continue;
    }
    // Wrap to next line if we hit right edge
    if (cx + Font->FontWidth >= SSD1306_WIDTH) {
      cx = 0;
      cy += Font->FontHeight;
    }
    // Stop if we run out of vertical space
    if (cy + Font->FontHeight >= SSD1306_HEIGHT) {
      return HAL_ERROR;
    }
    // Draw character
    SSD1306_Putc(cx, cy, *str, Font);
    // Advance cursor horizontally
    cx += Font->FontWidth;
    str++;
  }
  return HAL_OK;
}

void OLED_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    for (uint16_t i = x; i < x + w; i++)
    {
        SSD1306_SetPixel(i, y, SSD1306_COLOR_WHITE);
        SSD1306_SetPixel(i, y + h - 1, SSD1306_COLOR_WHITE);
    }

    for (uint16_t j = y; j < y + h; j++)
    {
        SSD1306_SetPixel(x, j, SSD1306_COLOR_WHITE);
        SSD1306_SetPixel(x + w - 1, j, SSD1306_COLOR_WHITE);
    }
}

void OLED_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    for (uint16_t j = y; j < y + h; j++)
    {
        for (uint16_t i = x; i < x + w; i++)
        {
            SSD1306_SetPixel(i, j, SSD1306_COLOR_WHITE);
        }
    }
}

void OLED_DrawText(uint16_t x, uint16_t y, char *str, FontDef_t *font)
{
    while (*str)
    {
        SSD1306_Putc(x, y, *str, font);
        x += font->FontWidth;
        str++;
    }
}

void OLED_ShowProgress(uint8_t percent)
{
    char buf[8];

    uint16_t bar_x = 14;
    uint16_t bar_y = 30;
    uint16_t bar_w = 100;
    uint16_t bar_h = 12;

    if (percent > 100) percent = 100;

    uint16_t fill_w = ((bar_w - 2) * percent) / 100;

    SSD1306_Fill(SSD1306_COLOR_BLACK);

    // Title
    OLED_DrawText(15, 8, "Printing ...", &Font_7x10);

    // Bar outline
    OLED_DrawRect(bar_x, bar_y, bar_w, bar_h);

    // Bar fill
    if (fill_w > 0)
    {
        OLED_FillRect(bar_x + 1, bar_y + 1, fill_w, bar_h - 2);
    }

    // Percentage text
    sprintf(buf, "%d%%", percent);
    OLED_DrawText(45, 48, buf, &Font_7x10);

    SSD1306_UpdateScreen();
}

void OLED_ShowCalibration(GPIO_PinState x_state, GPIO_PinState y_state)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    // Title
    OLED_DrawText(18, 8, "Calibrating...", &Font_7x10);

    // X axis status
    OLED_DrawText(10, 28, "X Axis:", &Font_7x10);

    if (x_state == GPIO_PIN_SET)
        OLED_DrawText(70, 28, "Done", &Font_7x10);
    else
        OLED_DrawText(70, 28, "Homing", &Font_7x10);

    // Y axis status
    OLED_DrawText(10, 42, "Y Axis:", &Font_7x10);

    if (y_state == GPIO_PIN_SET)
        OLED_DrawText(70, 42, "Done", &Font_7x10);
    else
        OLED_DrawText(70, 42, "Homing", &Font_7x10);

    SSD1306_UpdateScreen();
}

void OLED_ShowStartScreen(void)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    OLED_DrawText(22, 18, "Press button", &Font_7x10);
    OLED_DrawText(36, 34, "to start", &Font_7x10);

    SSD1306_UpdateScreen();
}

void OLED_ShowEndScreen(void)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    OLED_DrawText(10, 18, "Finished Printing!", &Font_7x10);
    //OLED_DrawText(36, 34, "to start", &Font_7x10);

    SSD1306_UpdateScreen();
}


