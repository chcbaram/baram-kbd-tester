#include "usbh.h"


#ifdef _USE_HW_USBH
#include "cli.h"

#ifdef _USE_HW_CLI
static void cliCmd(cli_args_t *args);
#endif





bool usbhInit(void)
{

#if 0
  GPIO_InitTypeDef GPIO_InitStruct;

 // USB Clock
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  RCC_PeriphCLKInitTypeDef usb_clk_init = { 0};
  usb_clk_init.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
  usb_clk_init.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_HSE;
  if (HAL_RCCEx_PeriphCLKConfig(&usb_clk_init) != HAL_OK) {
    Error_Handler();
  }

  /** Set the OTG PHY reference clock selection
  */
  HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);


  // IOSV bit MUST be set to access GPIO port G[2:15] */
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Configure USB GPIOs */
  /* Configure DM DP Pins */
  // GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
  // GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  // GPIO_InitStruct.Pull = GPIO_NOPULL;
  // GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  // GPIO_InitStruct.Alternate = GPIO_AF10_USB;
  // HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  

  __HAL_RCC_GPIOA_CLK_ENABLE();
  /**USB_OTG_HS GPIO Configuration
  PA8     ------> USB_OTG_HS_SOF
  PA11     ------> USB_OTG_HS_DM
  PA12     ------> USB_OTG_HS_DP
  */
  GPIO_InitStruct.Pin       = GPIO_PIN_8;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_USB_HS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USB clock enable */
  __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
  __HAL_RCC_USBPHYC_CLK_ENABLE();

  /* Enable USB power on Pwrctrl CR2 register */
  HAL_PWREx_EnableVddUSB();
  HAL_PWREx_EnableUSBHSTranceiverSupply();

  /*Configuring the SYSCFG registers OTG_HS PHY*/
  HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

  // Disable VBUS sense (B device)
  USB_OTG_HS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;

  // B-peripheral session valid override enable
  // USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_VBVALEXTOEN;
  // USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_VBVALOVAL;
#else
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
  PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_HSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /** Set the OTG PHY reference clock selection
   */
  HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

  __HAL_RCC_GPIOA_CLK_ENABLE();
  /**USB_OTG_HS GPIO Configuration
  PA8     ------> USB_OTG_HS_SOF
  PA11     ------> USB_OTG_HS_DM
  PA12     ------> USB_OTG_HS_DP
  */
  GPIO_InitStruct.Pin       = GPIO_PIN_8;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_USB_HS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USB_OTG_HS clock enable */
  __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
  __HAL_RCC_USBPHYC_CLK_ENABLE();

  /* Enable VDDUSB */
  if (__HAL_RCC_PWR_IS_CLK_DISABLED())
  {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_EnableVddUSB();

    /*configure VOSR register of USB*/
    HAL_PWREx_EnableUSBHSTranceiverSupply();
    __HAL_RCC_PWR_CLK_DISABLE();
  }
  else
  {
    HAL_PWREx_EnableVddUSB();

    /*configure VOSR register of USB*/
    HAL_PWREx_EnableUSBHSTranceiverSupply();
  }

  /*Configuring the SYSCFG registers OTG_HS PHY*/
  /*OTG_HS PHY enable*/
  HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

  // Disable VBUS sense (B device)
  USB_OTG_HS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;

  /* Enable USB PHY pulldown resistors */
  USB_OTG_HS->GCCFG |= USB_OTG_GCCFG_PULLDOWNEN;

  // /* USB_OTG_HS interrupt Init */
  HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
  // HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
#endif

  // init host stack on configured roothub port
  tusb_rhport_init_t host_init = {
    .role  = TUSB_ROLE_HOST,
    .speed = TUSB_SPEED_AUTO};

  tusb_init(BOARD_TUH_RHPORT, &host_init);

#ifdef _USE_HW_CLI
  cliAdd("usbh", cliCmd);
#endif
  return true;
}

bool usbhBegin(void)
{
  bool ret = true;

  return ret;
}

bool usbhUpdate(void)
{
  tuh_task();
  return true;
}

uint32_t tusb_time_millis_api(void)
{
  return millis();
}

size_t usbGeUniqueID(uint8_t id[], size_t max_len)
{
  (void)max_len;
  volatile uint32_t *stm32_uuid = (volatile uint32_t *)UID_BASE;
  uint32_t          *id32       = (uint32_t *)(uintptr_t)id;
  uint8_t const      len        = 12;

  id32[0] = stm32_uuid[0];
  id32[1] = stm32_uuid[1];
  id32[2] = stm32_uuid[2];

  return len;
}

size_t usbGetSerial(uint16_t desc_str1[], size_t max_chars)
{
  uint8_t uid[16] TU_ATTR_ALIGNED(4);
  size_t  uid_len;


  uid_len = usbGeUniqueID(uid, sizeof(uid));

  if (uid_len > max_chars / 2) uid_len = max_chars / 2;

  for (size_t i = 0; i < uid_len; i++)
  {
    for (size_t j = 0; j < 2; j++)
    {
      const char nibble_to_hex[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
      uint8_t const nibble       = (uid[i] >> (j * 4)) & 0xf;
      desc_str1[i * 2 + (1 - j)] = nibble_to_hex[nibble]; // UTF-16-LE
    }
  }

  return 2 * uid_len;
}

void OTG_HS_IRQHandler(void) {
  tuh_int_handler(0);
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