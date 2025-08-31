#include "ap.h"


static void cliTest(cli_args_t *args);



void apInit(void)
{  
  cliOpen(HW_UART_CH_CLI, 115200);
  logBoot(false);

  cliAdd("test", cliTest);
}

void apMain(void)
{
  uint32_t pre_time;

  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);
    }

    cliMain();
    usbhUpdate();
  }
}

volatile uint32_t key_pre_time;
volatile uint32_t key_exe_time;

void cliTest(cli_args_t *args)
{
  bool ret = false;
  uint32_t pre_time;


  if (args->argc == 1 && args->isStr(0, "key"))
  {
    key_pre_time = micros();
    gpioPinWrite(KEY_PRESS_A, _DEF_HIGH);
    pre_time = millis();
    while(millis()-pre_time <= 100)
    {
      usbhUpdate();
    }
    logPrintf("press key time   : %d us, %d.%d ms\n", key_exe_time, key_exe_time/1000, key_exe_time%1000);

    key_pre_time = micros();
    gpioPinWrite(KEY_PRESS_A, _DEF_LOW);
    pre_time = millis();
    while(millis()-pre_time <= 100)
    {
      usbhUpdate();
    }
    logPrintf("release key time : %d us, %d.%d ms\n", key_exe_time,key_exe_time/1000, key_exe_time%1000);

    ret = true;
  }


  if (!ret)
  {
    cliPrintf("test key\n");
  }
}