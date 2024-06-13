#include "usbh.h"


#ifdef _USE_HW_USBH
#include "cli.h"

#ifdef _USE_HW_CLI
static void cliCmd(cli_args_t *args);
#endif
static void usbhUserProcess(USBH_HandleTypeDef *phost, uint8_t id);

extern HCD_HandleTypeDef hhcd_USB_OTG_HS;
USBH_HandleTypeDef hUsbHostHS;




bool usbhInit(void)
{

#ifdef _USE_HW_CLI
  cliAdd("usbh", cliCmd);
#endif
  return true;
}

bool usbhBegin(void)
{
  bool ret = true;


  if (USBH_Init(&hUsbHostHS, usbhUserProcess, HOST_HS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_RegisterClass(&hUsbHostHS, USBH_HID_CLASS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_Start(&hUsbHostHS) != USBH_OK)
  {
    Error_Handler();
  }

  return ret;
}

bool usbhUpdate(void)
{
  USBH_Process(&hUsbHostHS);
  return true;
}

void usbhUserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  switch (id)
  {
    case HOST_USER_CONNECTION:
      logPrintf("HOST_USER_CONNECTION \n");
      break;

    case HOST_USER_DISCONNECTION:
      // Appli_state = APPLICATION_DISCONNECT;
      logPrintf("HOST_USER_DISCONNECTION \n");
      break;

    case HOST_USER_CLASS_ACTIVE:
      // Appli_state = APPLICATION_READY;
      logPrintf("HOST_USER_CLASS_ACTIVE \n");
      break;

    default:
      logPrintf("usbhUserProcess : %d\n", id);
      break;
  }
}


void OTG_HS_IRQHandler(void)
{
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_HS);
}


#ifdef _USE_HW_CLI
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    ret = true;
  }

  if (ret == false)
  {
    cliPrintf("usb info\n");
  }
}
#endif

#endif