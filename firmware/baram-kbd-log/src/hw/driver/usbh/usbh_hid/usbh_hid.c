#include "usbh_hid.h"

#include "tusb.h"
#include "cli.h"

#define MAX_REPORT 4

static uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII};

// Each HID instance can has multiple reports
static struct
{
  uint8_t               report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];


static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len);
static void cliCmd(cli_args_t *args);




bool usbHidInit(void)
{
  logPrintf("[OK] USBH Hid\n");
  cliAdd("usbhid", cliCmd);
  return true;
}

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len)
{
  logPrintf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

  // Interface protocol (hid_interface_protocol_enum_t)
  const char   *protocol_str[] = {"None", "Keyboard", "Mouse"};
  uint8_t const itf_protocol   = tuh_hid_interface_protocol(dev_addr, instance);

  logPrintf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  // By default, host stack will use boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if (itf_protocol == HID_ITF_PROTOCOL_NONE)
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    logPrintf("HID has %u reports \r\n", hid_info[instance].report_count);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if (!tuh_hid_receive_report(dev_addr, instance))
  {
    logPrintf("Error: cannot request to receive report\r\n");
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  logPrintf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      TU_LOG2("HID receive boot keyboard report\r\n");
      process_kbd_report((hid_keyboard_report_t const *)report);
      break;

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
      break;
  }

  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance))
  {
    logPrintf("Error: cannot request to receive report\r\n");
  }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
  for (uint8_t i = 0; i < 6; i++)
  {
    if (report->keycode[i] == keycode)
    {
      return true;
    }
  }
  return false;
}

extern bool     key_is_received;
extern uint32_t key_pre_time;
extern uint32_t key_exe_time;

static void process_kbd_report(hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released


  key_exe_time = micros() - key_pre_time;
  key_is_received = true;
  return;


  //------------- example code ignore control (non-printable) key affects -------------//
  for (uint8_t i = 0; i < 6; i++)
  {
    if (report->keycode[i])
    {
      if (find_key_in_report(&prev_report, report->keycode[i]))
      {
        // exist in previous report means the current key is holding
      }
      else
      {
        // not existed in previous report means the current key is pressed
        bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
        uint8_t    ch       = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];
        putchar(ch);
        if (ch == '\r')
        {
          putchar('\n');
        }

#ifndef __ICCARM__      // TODO IAR doesn't support stream control ?
        fflush(stdout); // flush right away, else nanolib will wait for newline
#endif
      }
    }
    // TODO example skips key released
  }

  prev_report = *report;
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
  (void)dev_addr;
  (void)len;

  uint8_t const          rpt_count    = hid_info[instance].report_count;
  tuh_hid_report_info_t *rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t *rpt_info     = NULL;

  if (rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }
  else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for (uint8_t i = 0; i < rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id)
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    printf("Couldn't find report info !\r\n");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if (rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP)
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        TU_LOG2("HID receive keyboard report\r\n");
        // Assume keyboard follow boot report layout
        process_kbd_report((hid_keyboard_report_t const *)report);
        break;

      case HID_USAGE_DESKTOP_MOUSE:
        TU_LOG2("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        // process_mouse_report((hid_mouse_report_t const *)report);
        break;

      default:
        printf("report[%u] ", rpt_info->report_id);
        for (uint8_t i = 0; i < len; i++)
        {
          printf("%02X ", report[i]);
        }
        printf("\r\n");
        break;
    }
  }
}

#ifdef _USE_HW_CLI
void cliCmd(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "info") == true)
  { 
    // cliPrintf("speed          : %d\n", p_host->device.speed);
    // cliPrintf("state          : %d\n", p_hid->state);
    // cliPrintf("ctl_state      : %d\n", p_hid->ctl_state);
    // cliPrintf("ep_addr        : 0x%02X\n", p_hid->ep_addr);
    // cliPrintf("wMaxPacketSize : %d\n", p_hid->length);
    // cliPrintf("poll           : %d\n", p_hid->poll);
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "log") == true)
  {
    // uint32_t pre_time;
    // uint32_t pre_cnt;

    // pre_cnt = p_host->Timer;
    // pre_time = millis();
    // while(cliKeepLoop())
    // {
    //   if (millis()-pre_time >= 1000)
    //   {
    //     pre_time = millis();
    //     cliPrintf("sof cnt : %d/sec\n", p_host->Timer - pre_cnt);

    //     pre_cnt = p_host->Timer;
    //   }
    // }
    // ret = true;
  }

  if (ret == false)
  {
    cliPrintf("usbhid info\n");
    cliPrintf("usbhid log\n");
  }
}
#endif
