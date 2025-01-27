/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32mp13xx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and
  *          peripherals interrupt service routine.
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
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern DMA_HandleTypeDef hdma_memtomem_dma1_stream0;
extern MDMA_HandleTypeDef hmdma_memtomem;
extern LTDC_HandleTypeDef hltdc;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/* STM32MP13xx Peripheral Interrupt Handlers                                  */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32mp135c_ca7.c).              */
/******************************************************************************/

/* USER CODE BEGIN 1 */
/**
  * @brief This function handles DMA1 stream0 global interrupt.
  */
void DMA1_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream0_IRQn 0 */

  /* USER CODE END DMA1_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_memtomem_dma1_stream0);
  /* USER CODE BEGIN DMA1_Stream0_IRQn 1 */

  /* USER CODE END DMA1_Stream0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line5 interrupt.
  */
void EXTI5_IRQHandler(void)
{
#ifdef HAL_EXTI_MODULE_ENABLED
  HAL_EXTI_IRQHandler(&hts_exti[0]);
#else
  BSP_TS_Callback(0);
#endif
}

/**
  * @brief This function handles EXTI line6 interrupt.
  */
void EXTI6_IRQHandler(void)
{
#ifdef HAL_EXTI_MODULE_ENABLED
  HAL_EXTI_IRQHandler(&hExtiButtonHandle[BUTTON_TAMPER]);
#else
  BSP_PB_IRQHandler(BUTTON_TAMPER);
#endif
}

/**
  * @brief This function handles EXTI line8 interrupt.
  */
void EXTI8_IRQHandler(void)
{
#ifdef HAL_EXTI_MODULE_ENABLED
  HAL_EXTI_IRQHandler(&hExtiButtonHandle[BUTTON_WAKEUP]);
#else
  BSP_PB_IRQHandler(BUTTON_WAKEUP);
#endif
}

/**
  * @brief This function handles EXTI line13 interrupt.
  */
void EXTI13_IRQHandler(void)
{
#ifdef HAL_EXTI_MODULE_ENABLED
  HAL_EXTI_IRQHandler(&hExtiButtonHandle[BUTTON_USER2]);
#else
  BSP_PB_IRQHandler(BUTTON_USER2);
#endif
}

/**
  * @brief This function handles EXTI line14 interrupt.
  */
void EXTI14_IRQHandler(void)
{
#ifdef HAL_EXTI_MODULE_ENABLED
  HAL_EXTI_IRQHandler(&hExtiButtonHandle[BUTTON_USER]);
#else
  BSP_PB_IRQHandler(BUTTON_USER);
#endif
}

/**
* @brief This function handles MDMA global interrupt in Non-SECURE Mode.
*/
void MDMA_IRQHandler(void)
{
  /* Check the interrupt and clear flag */
  HAL_MDMA_IRQHandler(&hmdma_memtomem);
}

/**
  * @brief  This function handles LTDC global interrupt request.
  * @param  None
  * @retval None
  */
void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hltdc);
}
/* USER CODE END 1 */
