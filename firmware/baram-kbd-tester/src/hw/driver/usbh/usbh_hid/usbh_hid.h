
#ifndef __USBH_HID_H
#define __USBH_HID_H

#ifdef __cplusplus
extern "C" {
#endif


#include "hw_def.h"
#include "tusb.h"


typedef struct
{
  bool is_high_speed;
  char manufacturer[256];
  char product[256];
} usbh_hid_info_t;


bool usbhHidIsConnected(void);
void usbhHidGetInfo(usbh_hid_info_t *info);
void usbhHidSetReportCB( void (*func)(hid_keyboard_report_t const *));

#ifdef __cplusplus
}
#endif

#endif /* __USBH_HID_H */
