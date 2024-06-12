/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pssi.c
  * @brief   This file provides code for the configuration
  *          of the PSSI instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "pssi.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

PSSI_HandleTypeDef hpssi;

/* PSSI init function */
void MX_PSSI_Init(void)
{

  /* USER CODE BEGIN PSSI_Init 0 */

  /* USER CODE END PSSI_Init 0 */

  /* USER CODE BEGIN PSSI_Init 1 */

  /* USER CODE END PSSI_Init 1 */
  hpssi.Instance = PSSI;
  hpssi.Init.DataWidth = HAL_PSSI_8BITS;
  hpssi.Init.BusWidth = HAL_PSSI_8LINES;
  hpssi.Init.ControlSignal = HAL_PSSI_DE_ENABLE;
  hpssi.Init.ClockPolarity = HAL_PSSI_FALLING_EDGE;
  hpssi.Init.DataEnablePolarity = HAL_PSSI_DEPOL_ACTIVE_HIGH;
  hpssi.Init.ReadyPolarity = HAL_PSSI_RDYPOL_ACTIVE_LOW;
  if (HAL_PSSI_Init(&hpssi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN PSSI_Init 2 */

  /* USER CODE END PSSI_Init 2 */

}

void HAL_PSSI_MspInit(PSSI_HandleTypeDef* pssiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(pssiHandle->Instance==PSSI)
  {
  /* USER CODE BEGIN PSSI_MspInit 0 */

  /* USER CODE END PSSI_MspInit 0 */
    /* PSSI clock enable */
    __HAL_RCC_DCMI_PSSI_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**PSSI GPIO Configuration
    PA4     ------> PSSI_DE
    PA6     ------> PSSI_PDCK
    PC6     ------> PSSI_D0
    PC7     ------> PSSI_D1
    PC8     ------> PSSI_D2
    PC9     ------> PSSI_D3
    PC11     ------> PSSI_D4
    PB6     ------> PSSI_D5
    PB7     ------> PSSI_RDY
    PB8     ------> PSSI_D6
    PB9     ------> PSSI_D7
    */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF10_PSSI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_PSSI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF10_PSSI;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_PSSI;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF10_PSSI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PSSI interrupt Init */
    HAL_NVIC_SetPriority(DCMI_PSSI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DCMI_PSSI_IRQn);
  /* USER CODE BEGIN PSSI_MspInit 1 */

  /* USER CODE END PSSI_MspInit 1 */
  }
}

void HAL_PSSI_MspDeInit(PSSI_HandleTypeDef* pssiHandle)
{

  if(pssiHandle->Instance==PSSI)
  {
  /* USER CODE BEGIN PSSI_MspDeInit 0 */

  /* USER CODE END PSSI_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_DCMI_PSSI_CLK_DISABLE();

    /**PSSI GPIO Configuration
    PA4     ------> PSSI_DE
    PA6     ------> PSSI_PDCK
    PC6     ------> PSSI_D0
    PC7     ------> PSSI_D1
    PC8     ------> PSSI_D2
    PC9     ------> PSSI_D3
    PC11     ------> PSSI_D4
    PB6     ------> PSSI_D5
    PB7     ------> PSSI_RDY
    PB8     ------> PSSI_D6
    PB9     ------> PSSI_D7
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9
                          |GPIO_PIN_11);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9);

    /* PSSI interrupt Deinit */
    HAL_NVIC_DisableIRQ(DCMI_PSSI_IRQn);
  /* USER CODE BEGIN PSSI_MspDeInit 1 */

  /* USER CODE END PSSI_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
