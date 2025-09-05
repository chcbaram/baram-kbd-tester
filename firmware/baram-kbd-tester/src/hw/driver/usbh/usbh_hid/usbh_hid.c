#include "usbh_hid.h"

#include "tusb.h"
#include "cli.h"


#define MAX_REPORT 4
// English
#define LANGUAGE_ID 0x0409



static uint8_t const keycode2ascii[128][2] = {HID_KEYCODE_TO_ASCII};

// Each HID instance can has multiple reports
static struct
{
  uint8_t               report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

CFG_TUH_MEM_SECTION struct {
  TUH_EPBUF_TYPE_DEF(tusb_desc_device_t, device);
  TUH_EPBUF_DEF(serial, 64*sizeof(uint16_t));
  TUH_EPBUF_DEF(buf, 128*sizeof(uint16_t));
} desc;

static bool is_connected = false;
static usbh_hid_info_t usbh_hid_info;
static void (*report_func_cb)(hid_keyboard_report_t const *) = NULL;

static void process_kbd_report(hid_keyboard_report_t const *report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len);
static void print_utf16(uint16_t* temp_buf, size_t buf_len);
static void cliCmd(cli_args_t *args);




bool usbHidInit(void)
{
  logPrintf("[OK] USBH Hid\n");
  cliAdd("usbhid", cliCmd);
  return true;
}

bool usbhHidIsConnected(void)
{
  return is_connected;
}

void usbhHidGetInfo(usbh_hid_info_t *info)
{
  *info = usbh_hid_info;
}

void usbhHidSetReportCB( void (*func)(hid_keyboard_report_t const *))
{
  report_func_cb = func;
}

void tuh_mount_cb(uint8_t daddr)
{
  // Get Device Descriptor
  uint8_t xfer_result = tuh_descriptor_get_device_sync(daddr, &desc.device, 18);
  if (XFER_RESULT_SUCCESS != xfer_result)
  {
    logPrintf("Failed to get device descriptor\r\n");
    return;
  }

  logPrintf("Device %u: ID %04x:%04x SN ", daddr, desc.device.idVendor, desc.device.idProduct);

  xfer_result = XFER_RESULT_FAILED;
  if (desc.device.iSerialNumber != 0)
  {
    xfer_result = tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, desc.serial, sizeof(desc.serial));
  }
  if (XFER_RESULT_SUCCESS != xfer_result)
  {
    uint16_t *serial = (uint16_t *)(uintptr_t)desc.serial;
    serial[0]        = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * 3 + 2));
    serial[1]        = 'n';
    serial[2]        = '/';
    serial[3]        = 'a';
    serial[4]        = 0;
  }
  print_utf16((uint16_t *)(uintptr_t)desc.serial, sizeof(desc.serial) / 2);
  logPrintf("\r\n");

  logPrintf("Device Descriptor:\r\n");
  logPrintf("  bLength             %u\r\n", desc.device.bLength);
  logPrintf("  bDescriptorType     %u\r\n", desc.device.bDescriptorType);
  logPrintf("  bcdUSB              %04x\r\n", desc.device.bcdUSB);
  logPrintf("  bDeviceClass        %u\r\n", desc.device.bDeviceClass);
  logPrintf("  bDeviceSubClass     %u\r\n", desc.device.bDeviceSubClass);
  logPrintf("  bDeviceProtocol     %u\r\n", desc.device.bDeviceProtocol);
  logPrintf("  bMaxPacketSize0     %u\r\n", desc.device.bMaxPacketSize0);
  logPrintf("  idVendor            0x%04x\r\n", desc.device.idVendor);
  logPrintf("  idProduct           0x%04x\r\n", desc.device.idProduct);
  logPrintf("  bcdDevice           %04x\r\n", desc.device.bcdDevice);

  // Get String descriptor using Sync API

  
  usbh_hid_info.manufacturer[0] = 0;
  logPrintf("  iManufacturer       %u     ", desc.device.iManufacturer);
  if (desc.device.iManufacturer != 0)
  {
    xfer_result = tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, desc.buf, sizeof(desc.buf));
    if (XFER_RESULT_SUCCESS == xfer_result)
    {
      print_utf16((uint16_t *)(uintptr_t)desc.buf, sizeof(desc.buf) / 2);

      strncpy(usbh_hid_info.manufacturer, (const char *)desc.buf, 256);
    }
  }
  logPrintf("\r\n");

  usbh_hid_info.product[0] = 0;
  logPrintf("  iProduct            %u     ", desc.device.iProduct);
  if (desc.device.iProduct != 0)
  {
    xfer_result = tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, desc.buf, sizeof(desc.buf));
    if (XFER_RESULT_SUCCESS == xfer_result)
    {
      print_utf16((uint16_t *)(uintptr_t)desc.buf, sizeof(desc.buf) / 2);
      strncpy(usbh_hid_info.product, (const char *)desc.buf, 256);
    }
  }
  logPrintf("\r\n");

  logPrintf("  iSerialNumber       %u     ", desc.device.iSerialNumber);
  logPrintf((char *)desc.serial); // serial is already to UTF-8
  logPrintf("\r\n");

  logPrintf("  bNumConfigurations  %u\r\n", desc.device.bNumConfigurations);
}

// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr)
{
  logPrintf("Device removed, address = %d\r\n", daddr);
}

//--------------------------------------------------------------------+
// String Descriptor Helper
//--------------------------------------------------------------------+

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len)
{
  // TODO: Check for runover.
  (void)utf8_len;
  // Get the UTF-16 length out of the data itself.

  for (size_t i = 0; i < utf16_len; i++)
  {
    uint16_t chr = utf16[i];
    if (chr < 0x80)
    {
      *utf8++ = chr & 0xffu;
    }
    else if (chr < 0x800)
    {
      *utf8++ = (uint8_t)(0xC0 | (chr >> 6 & 0x1F));
      *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
    }
    else
    {
      // TODO: Verify surrogate.
      *utf8++ = (uint8_t)(0xE0 | (chr >> 12 & 0x0F));
      *utf8++ = (uint8_t)(0x80 | (chr >> 6 & 0x3F));
      *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len)
{
  size_t total_bytes = 0;
  for (size_t i = 0; i < len; i++)
  {
    uint16_t chr = buf[i];
    if (chr < 0x80)
    {
      total_bytes += 1;
    }
    else if (chr < 0x800)
    {
      total_bytes += 2;
    }
    else
    {
      total_bytes += 3;
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
  return (int)total_bytes;
}

static void print_utf16(uint16_t *temp_buf, size_t buf_len)
{
  if ((temp_buf[0] & 0xff) == 0) return; // empty
  size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
  size_t utf8_len  = (size_t)_count_utf8_bytes(temp_buf + 1, utf16_len);
  _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *)temp_buf, sizeof(uint16_t) * buf_len);
  ((uint8_t *)temp_buf)[utf8_len] = '\0';

  logPrintf("%s", (char *)temp_buf);
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

  if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD)
  {
    is_connected = true;
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  logPrintf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);

  is_connected = false;
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

static void process_kbd_report(hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = {0, 0, {0}}; // previous report to check key released

  if (report_func_cb != NULL)
  {
    report_func_cb(report);
  }
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
    logPrintf("Couldn't find report info !\r\n");
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
        logPrintf("report[%u] ", rpt_info->report_id);
        for (uint8_t i = 0; i < len; i++)
        {
          logPrintf("%02X ", report[i]);
        }
        logPrintf("\r\n");
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
