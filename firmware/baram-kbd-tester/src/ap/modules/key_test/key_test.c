#include "key_test.h"


#define KEY_LOG_MAX   1000


typedef struct
{
  float pressed_time;
  float released_time;
} key_time_log_t;

typedef struct
{
  uint32_t pressed_us;
  uint32_t released_us;
} key_test_info_t;


static void cliTest(cli_args_t *args);
static void keyTestGUI(void);
static bool keyTestReq(key_test_info_t *test_info, uint32_t press_time_ms);
static void keyTestRun(void);
static void keyTestReportCB(hid_keyboard_report_t const *report);

volatile bool     key_is_received;
volatile uint32_t key_pre_time;
volatile uint32_t key_exe_time;

static uint16_t key_time_log_count = 0;
static key_time_log_t key_time_log[KEY_LOG_MAX];

static uint8_t btn_menu  = _DEF_BUTTON2;
static uint8_t btn_enter = _DEF_BUTTON2;
static uint8_t btn_left  = _DEF_BUTTON1;
static uint8_t btn_right = _DEF_BUTTON3;


static uint16_t key_test_cfg_press_time = 50;
static uint16_t key_test_cfg_press_count = 100;



bool keyTestInit(void)
{
  usbhHidSetReportCB(keyTestReportCB);

  cliAdd("test", cliTest);
  return true;
}

void keyTestUpdate(void)
{
  keyTestGUI();
}

void keyTestGUI(void)
{
  int16_t         x_offset = 10;
  static uint8_t  menu     = 0;
  uint8_t         menu_max = 4;
  uint8_t         menu_cur = 0;


  if (!lcdIsInit())
  {
    return;
  }

  if (buttonGetPressed(btn_left))
  {
    delay(10);
    while(buttonGetPressed(btn_left));
    
    if (menu == 0)
      menu = menu_max - 1;
    else      
      menu = menu - 1;
  }
  if (buttonGetPressed(btn_right))
  {
    delay(10);
    while(buttonGetPressed(btn_right));
    
    menu = (menu + 1) % menu_max;
  }
  menu_cur = menu;


  if (buttonGetPressed(btn_enter))
  {
    delay(10);
    while(buttonGetPressed(btn_enter));
    
    switch(menu_cur)
    {
      case 1:
        keyTestRun();
        break;
    }
  }

  if (lcdDrawAvailable())
  {
    lcdClearBuffer(black);

    lcdDrawRect(0, 0, 4, 32, white);
    for (int i=0; i<menu_max; i++)
    {
      if (i == menu)
        lcdDrawFillRect(0, i*(32/menu_max), 4, (32/menu_max), white);
    }

    if (menu_cur == 0)
    {
      if (usbhHidIsConnected())
      {
        usbh_hid_info_t usbh_info;

        usbhHidGetInfo(&usbh_info);
        
        lcdPrintf(x_offset,  0, white, "%s", usbh_info.manufacturer);        
        lcdPrintf(x_offset, 16, white, "%s", usbh_info.product);    
      }
      else
      {
        lcdPrintf(x_offset, 8, white, "Not Connected");        
      }
    }

    if (menu_cur == 1)
    {
      lcdPrintf(x_offset, 8, white, "- START TEST -");
    }

    if (menu_cur == 2)
    {
      lcdPrintf(x_offset, 0, white, "Press Time");
      lcdPrintf(x_offset,16, white, "%d ms", key_test_cfg_press_time);
    }    

    if (menu_cur == 3)
    {
      lcdPrintf(x_offset, 0, white, "Press Count");
      lcdPrintf(x_offset,16, white, "%d", key_test_cfg_press_count);
    }    

    lcdRequestDraw();
  }
}

void keyTestRun(void)
{
  uint16_t        percent;
  key_test_info_t test_info;
  uint32_t        press_time  = key_test_cfg_press_time;
  uint16_t        count_max   = key_test_cfg_press_count;
  uint32_t        time_val[2] = {
    0,
  };
  uint32_t time_max[2] = {
    0,
  };
  uint32_t time_min[2] = {0xFFFFFFFF, 0xFFFFFFFF};
  uint32_t time_sum[2] = {
    0,
  };
  uint32_t time_avg[2] = {
    0,
  };
  bool is_test_ok = true;

  key_time_log_count = 0;

  for (int i = 0; i < count_max; i++)
  {
    if (keyTestReq(&test_info, press_time))
    {
      time_val[0] = test_info.pressed_us;
      time_val[1] = test_info.released_us;

      for (int j = 0; j < 2; j++)
      {
        time_sum[j] += time_val[j];
        if (time_val[j] > time_max[j])
          time_max[j] = time_val[j];
        if (time_val[j] < time_min[j])
          time_min[j] = time_val[j];

        time_avg[j] = time_sum[j] / (i + 1);
      }
      key_time_log[i].pressed_time  = (float)test_info.pressed_us / 1000.f;
      key_time_log[i].released_time = (float)test_info.released_us / 1000.f;
      key_time_log_count            = i + 1;
    }
    else
    {
      lcdClearBuffer(black);
      lcdPrintf(16, 8, white, "Test Fail");
      lcdRequestDraw();
      is_test_ok = false;
      break;
    }

    usbhUpdate();

    percent = ((i + 1) * 100) / (count_max);
    lcdClearBuffer(black);
    lcdPrintf(96, 0, white, "%3d%%", percent);
    lcdDrawRect(0, 16, 128, 16, white);
    lcdDrawFillRect(2, 19, percent * 124 / 100, 10, white);
    lcdRequestDraw();

    usbhUpdate();
  }

  uint8_t view_line = 0;

  while(!buttonGetPressed(btn_menu))
  {    
    if (buttonGetPressed(btn_right))
    {
      delay(10);
      while(buttonGetPressed(btn_right));
      
      view_line = (view_line + 1) % 3;
    }

    if (is_test_ok)
    {
      lcdClearBuffer(black);

      switch(view_line)
      {
        case 0:
          lcdPrintf(0, 0, white, "P Avg %d.%03d ms",
                    time_avg[0] / 1000, time_avg[0] % 1000);
          lcdPrintf(0,16, white, "R Avg %d.%03d ms",
                    time_avg[1] / 1000, time_avg[1] % 1000);          
          break;

        case 1:
          lcdPrintf(0, 0, white, "P Max %d.%03d ms",
                    time_max[0] / 1000, time_max[0] % 1000);
          lcdPrintf(0,16, white, "R Max %d.%03d ms",
                    time_max[1] / 1000, time_max[1] % 1000);          
          break;

        case 2:
          lcdPrintf(0, 0, white, "P Min %d.%03d ms",
                    time_min[0] / 1000, time_min[0] % 1000);
          lcdPrintf(0,16, white, "R Min %d.%03d ms",
                    time_min[1] / 1000, time_min[1] % 1000);          
          break;
      }
      lcdRequestDraw();
    }
  }

  delay(10);
  while(buttonGetPressed(btn_menu));  
  delay(100);
}

void keyTestReportCB(hid_keyboard_report_t const *report)
{
  key_exe_time = micros() - key_pre_time;
  key_is_received = true;
}

bool keyTestReq(key_test_info_t *test_info, uint32_t press_time_ms)
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
      if (keyTestReq(&test_info, press_time))
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