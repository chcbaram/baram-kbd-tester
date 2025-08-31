#include "ap.h"


#define KEY_LOG_MAX   1000


typedef struct
{
  float pressed_time;
  float released_time;
} key_time_log_t;


static void cliTest(cli_args_t *args);

static uint16_t key_time_log_count = 0;
static key_time_log_t key_time_log[KEY_LOG_MAX];



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

volatile bool     key_is_received;
volatile uint32_t key_pre_time;
volatile uint32_t key_exe_time;

typedef struct
{
  uint32_t pressed_us;
  uint32_t released_us;
} key_test_info_t;

bool keyReqTest(key_test_info_t *test_info, uint32_t press_time_ms)
{
  uint32_t pre_time;
  bool ret = true;

  key_is_received = false;  
  gpioPinWrite(KEY_PRESS_A, _DEF_HIGH);
  key_pre_time = micros();

  pre_time = millis();  
  while(millis()-pre_time <= press_time_ms)
  {
    usbhUpdate();
  }
  if (key_is_received)
    test_info->pressed_us = key_exe_time;
  else
    ret = false;


  key_is_received = false;  
  gpioPinWrite(KEY_PRESS_A, _DEF_LOW);
  key_pre_time = micros();

  pre_time = millis();
  while(millis()-pre_time <= press_time_ms)
  {
    usbhUpdate();
  }

  if (key_is_received)
    test_info->released_us = key_exe_time;
  else
    ret = false;

  return ret;
}



void cliTest(cli_args_t *args)
{
  bool ret = false;
  


  if (args->argc >= 1 && args->isStr(0, "key"))
  {
    key_test_info_t test_info;
    uint32_t press_time = 100;
    uint16_t count_max = 1;
    uint32_t time_val[2] = {0, };
    uint32_t time_max[2] = {0, };
    uint32_t time_min[2] = {0xFFFFFFFF, 0xFFFFFFFF};
    uint32_t time_sum[2] = {0, };
    uint32_t time_avg[2] = {0, };
    bool is_test_ok = true;

    if (args->argc >= 2)
    {
      press_time = constrain(args->getData(1), 1, 10000);
    }
    if (args->argc >= 3)
    {
      count_max = constrain(args->getData(2), 1, 10000);
    }

    key_time_log_count = 0;
    cliPrintf("press_time  : %d ms\n", press_time);
    cliPrintf("press_count : %d \n", count_max);

    for (int i=0; i<count_max; i++)
    {
      if (keyReqTest(&test_info, press_time))
      {
        time_val[0] = test_info.pressed_us;
        time_val[1] = test_info.released_us; 

        for (int j=0; j<2; j++)
        {
          time_sum[j] += time_val[j];
          if (time_val[j] > time_max[j])
            time_max[j] = time_val[j];
          if (time_val[j] < time_min[j])
            time_min[j] = time_val[j];

          time_avg[j] = time_sum[j] / (i+1);
        }
        key_time_log[i].pressed_time = (float)test_info.pressed_us / 1000.f;
        key_time_log[i].released_time = (float)test_info.released_us / 1000.f;
        key_time_log_count = i + 1;
      }
      else
      {
        cliPrintf("keyReqTest() Fail\n");
        is_test_ok = false;
        break;
      }

      usbhUpdate();
      cliPrintf("progress : %d %%\r", ((i+1) * 100) / (count_max));
      usbhUpdate();
    }
    cliPrintf("\n");

    if (is_test_ok)
    {
      cliPrintf("press key time   : avg %d.%03d ms, max %d.%03d ms, min %d.%03d ms\n",                
                time_avg[0] / 1000, time_avg[0] % 1000,
                time_max[0] / 1000, time_max[0] % 1000,
                time_min[0] / 1000, time_min[0] % 1000
              );
      cliPrintf("release key time : avg %d.%03d ms, max %d.%03d ms, min %d.%03d ms\n",
                time_avg[1] / 1000, time_avg[1] % 1000,
                time_max[1] / 1000, time_max[1] % 1000,
                time_min[1] / 1000, time_min[1] % 1000
              );
    }

    ret = true;
  }

  if (args->argc >= 1 && args->isStr(0, "log"))
  {
    uint32_t log_show_cnt = key_time_log_count;

    if (args->argc == 2)
    {
      log_show_cnt = cmin(args->getData(1), key_time_log_count);
    }
    cliPrintf("log_show_cnt : %d\n", log_show_cnt);

    for (int i=0; i<log_show_cnt; i++)
    {
      cliPrintf(">o-t: %f\n", 0.f);
      cliPrintf(">p-t: %f\n", key_time_log[i].pressed_time);
      cliPrintf(">r-t: %f\n", key_time_log[i].released_time);
    }
    ret = true;
  }

  if (!ret)
  {
    cliPrintf("test key [time] [count]\n");
    cliPrintf("test log\n");
  }
}