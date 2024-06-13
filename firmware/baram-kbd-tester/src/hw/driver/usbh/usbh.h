#ifndef USBH_H_
#define USBH_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "hw_def.h"

#ifdef _USE_HW_USBH

#include "usbh_core.h"

#include "usbh_hid.h"


bool usbhInit(void);
void usbhDeInit(void);
bool usbhBegin(void);
bool usbhUpdate(void);

#endif


#ifdef __cplusplus
}
#endif

#endif 
