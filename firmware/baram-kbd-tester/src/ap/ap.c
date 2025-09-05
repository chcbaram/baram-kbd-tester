#include "ap.h"
#include "key_test/key_test.h"


static void updateCLI(void);
static void updateLOGO(void);
static void updateLED(void);
static void updateUSB(void);
static void updateMODULE(void);




void apInit(void)
{  
  cliOpen(HW_UART_CH_CLI, 115200);
  logBoot(false);

  updateLOGO();
  keyTestInit();
}

void apMain(void)
{
  while(1)
  {
    updateCLI();
    updateUSB();
    updateLED();
    updateMODULE();
  }
}

void updateCLI(void)
{
  cliMain();
}

void updateUSB(void)
{
  usbhUpdate();
}

void updateLOGO(void)
{
  for (int i = 0; i < 32; i += 1) 
  {
    lcdClearBuffer(black);
    lcdPrintfResize(0, 40 - i, green, 16, "  -- BARAM --");
    lcdDrawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, white);
    lcdUpdateDraw();
    delay(10);
  }  
  delay(500);
  lcdClear(black);
}

void updateLED(void)
{
  static uint32_t pre_time = 0;
  
  
  if (millis() - pre_time >= 500)
  {
    pre_time = millis();
    ledToggle(_DEF_LED1);
  }
}

void updateMODULE(void)
{
  keyTestUpdate();
}