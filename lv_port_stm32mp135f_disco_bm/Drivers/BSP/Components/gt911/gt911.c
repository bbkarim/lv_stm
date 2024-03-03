/**
  ******************************************************************************
  * @file    gt911.c
  * @author  MCD Application Team
  * @brief   This file provides a set of functions needed to manage the GT911
  *          devices.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "gt911.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup Component
  * @{
  */

/** @defgroup GT911 GT911
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/** @defgroup GT911_Exported_Variables GT911 Exported Variables
  * @{
  */

/* Touch screen driver structure initialization */
GT911_TS_Drv_t GT911_TS_Driver =
{
  GT911_Init,
  GT911_DeInit,
  GT911_GestureConfig,
  GT911_ReadID,
  GT911_GetState,
  GT911_GetMultiTouchState,
  GT911_GetGesture,
  GT911_GetCapabilities,
  GT911_EnableIT,
  GT911_DisableIT,
  GT911_ClearIT,
  GT911_ITStatus
};
/**
  * @}
  */

/** @defgroup GT911_Private_Function_Prototypes GT911 Private Function Prototypes
  * @{
  */
#if (GT911_AUTO_CALIBRATION_ENABLED == 1)
static int32_t GT911_TS_Calibration(GT911_Object_t *pObj);
static int32_t GT911_Delay(GT911_Object_t *pObj, uint32_t Delay);
#endif /* GT911_AUTO_CALIBRATION_ENABLED == 1 */
static int32_t GT911_DetectTouch(GT911_Object_t *pObj);
static int32_t ReadRegWrap(void *handle, uint16_t Reg, uint8_t *pData, uint16_t Length);
static int32_t WriteRegWrap(void *handle, uint16_t Reg, uint8_t *pData, uint16_t Length);

/**
  * @}
  */

/** @defgroup GT911_Exported_Functions GT911 Exported Functions
  * @{
  */

/**
  * @brief  Register IO bus to component object
  * @param  pObj Component object pointer
  * @param  pIO IO bus pointer
  * @retval Component status
  */
int32_t GT911_RegisterBusIO (GT911_Object_t *pObj, GT911_IO_t *pIO)
{
  int32_t ret;

  if (pObj == NULL)
  {
    ret = GT911_ERROR;
  }
  else
  {
    pObj->IO.Init      = pIO->Init;
    pObj->IO.DeInit    = pIO->DeInit;
    pObj->IO.Address   = pIO->Address;
    pObj->IO.WriteReg  = pIO->WriteReg;
    pObj->IO.ReadReg   = pIO->ReadReg;
    pObj->IO.GetTick   = pIO->GetTick;

    pObj->Ctx.ReadReg  = ReadRegWrap;
    pObj->Ctx.WriteReg = WriteRegWrap;
    pObj->Ctx.handle   = pObj;

    if(pObj->IO.Init != NULL)
    {
      ret = pObj->IO.Init();
    }
    else
    {
      ret = GT911_ERROR;
    }
  }

  return ret;
}

/**
  * @brief  Get GT911 sensor capabilities
  * @param  pObj Component object pointer
  * @param  pCapabilities GT911 sensor capabilities pointer
  * @retval Component status
  */
int32_t GT911_GetCapabilities(GT911_Object_t *pObj, GT911_Capabilities_t *pCapabilities)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  /* Store component's capabilities */
  pCapabilities->MultiTouch = 1;
  pCapabilities->Gesture    = 1;
  pCapabilities->MaxTouch   = GT911_MAX_NB_TOUCH;
  pCapabilities->MaxXl      = GT911_MAX_X_LENGTH;
  pCapabilities->MaxYl      = GT911_MAX_Y_LENGTH;

  return GT911_OK;
}

/**
  * @brief  Initialize the GT911 communication bus
  *         from MCU to GT911 : ie I2C channel initialization (if required).
  * @param  pObj Component object pointer
  * @retval Component status
  */
int32_t GT911_Init(GT911_Object_t *pObj)
{
  int32_t ret = GT911_OK;

  if(pObj->IsInitialized == 0U)
  {
    /* Initialize IO BUS layer */
    pObj->IO.Init();

#if (GT911_AUTO_CALIBRATION_ENABLED == 1)
    /* Hw Calibration sequence start : should be done once after each power up */
    /* This is called internal calibration of the touch screen                 */
    ret += GT911_TS_Calibration(pObj);
#endif /* (GT911_AUTO_CALIBRATION_ENABLED == 1) */
    pObj->IsInitialized = 1;
  }
#if (GT911_AUTO_CALIBRATION_ENABLED == 1)
  if( ret==GT911_OK )
#endif /* (GT911_AUTO_CALIBRATION_ENABLED == 1) */
  {
    uint16_t ii;
    uint8_t  r_data;
    uint8_t  calc_chksum = 0;
    /* Calculate the CRC of the configuration registers */
    for( ii=0x8047U; ii<0x80FFU; ii++ )
    {
      if( pObj->IO.ReadReg( pObj->IO.Address, ii, &r_data, 1 )!=0 )
      {
        ret = GT911_ERROR;  // I2C reading failed
        break;
      }
      else
      {
        calc_chksum += r_data;
      }
    }
    if( ret==GT911_OK )
    {
      calc_chksum = ((~calc_chksum)+1U)&0xffU;
      if( pObj->IO.ReadReg( pObj->IO.Address, 0x80FF, &r_data, 1 )!=0 )
      {
        ret = GT911_ERROR;  // I2C reading failed
      }
      else
      {
        if( calc_chksum!=r_data )
        {
          ret = GT911_ERROR;  // Wrong CheckSum
        }
      }
    }
  }

  if(ret != GT911_OK)
  {
    ret = GT911_ERROR;
  }

  return ret;
}

/**
  * @brief  De-Initialize the GT911 communication bus
  *         from MCU to GT911 : ie I2C channel initialization (if required).
  * @param  pObj Component object pointer
  * @retval Component status
  */
int32_t GT911_DeInit(GT911_Object_t *pObj)
{
  int32_t ret = GT911_OK;

  if(pObj->IsInitialized == 1U)
  {
    pObj->IsInitialized = 0;
  }

  return ret;
}

/**
  * @brief  Configure the GT911 gesture
  * @param  pObj  Component object pointer
  * @param  pGestureInit Gesture init structure pointer
  * @retval Component status
  */
int32_t GT911_GestureConfig(GT911_Object_t *pObj, GT911_Gesture_Init_t *pGestureInit)
{
  int32_t ret;

  ret = gt911_radian_value(&pObj->Ctx, (uint8_t)pGestureInit->Radian);
  ret += gt911_offset_left_right(&pObj->Ctx, (uint8_t)pGestureInit->OffsetLeftRight);
  ret += gt911_offset_up_down(&pObj->Ctx, (uint8_t)pGestureInit->OffsetUpDown);
  ret += gt911_disatnce_left_right(&pObj->Ctx, (uint8_t)pGestureInit->DistanceLeftRight);
  ret += gt911_distance_up_down(&pObj->Ctx, (uint8_t)pGestureInit->DistanceUpDown);
  ret += gt911_distance_zoom(&pObj->Ctx, (uint8_t)pGestureInit->DistanceZoom);

  if(ret != GT911_OK)
  {
    ret = GT911_ERROR;
  }

  return ret;
}

/**
  * @brief  Read the GT911 device ID, pre initialize I2C in case of need to be
  *         able to read the GT911 device ID, and verify this is a GT911.
  * @param  pObj Component object pointer
  * @param  pId Device ID (two bytes) pointer
  * @retval Component status
  */
int32_t GT911_ReadID(GT911_Object_t *pObj, volatile uint32_t *pId)
{
  return gt911_chip_id(&pObj->Ctx, (uint8_t *)pId);
}

/**
  * @brief  Get the touch screen X and Y positions values
  * @param  pObj Component object pointer
  * @param  pState Single Touch structure pointer
  * @retval Component status
  */
int32_t GT911_GetState(GT911_Object_t *pObj, GT911_State_t *pState)
{
  int32_t ret = GT911_OK;
  uint8_t  data[6];
  uint8_t value=0;

  pState->TouchDetected = (uint32_t)GT911_DetectTouch(pObj);
  if(pState->TouchDetected > GT911_MAX_NB_TOUCH)
  {
    ret = GT911_ERROR;
  }
  else
  {
    if( pState->TouchDetected==0U )
    {}  // No Touch Detected
    else if(gt911_read_reg(&pObj->Ctx, GT911_P1_XL_REG, data, (uint16_t)sizeof(data)) != GT911_OK)
    {
      ret = GT911_ERROR;
    }
    else
    {
      /* Send back first ready X position to caller */
      pState->TouchX = (((uint32_t)data[1] & GT911_P1_XH_TP_BIT_MASK) << 8) | ((uint32_t)data[0] & GT911_P1_XL_TP_BIT_MASK);
      /* Send back first ready Y position to caller */
      pState->TouchY = (((uint32_t)data[3] & GT911_P1_YH_TP_BIT_MASK) << 8) | ((uint32_t)data[2] & GT911_P1_YL_TP_BIT_MASK);
      // Acknowledge the reading of the Touch coordinates
      ret = gt911_write_reg(&pObj->Ctx, GT911_TD_STAT_REG, &value, 1);
    }
  }

  return ret;
}

/**
  * @brief  Get the touch screen Xn and Yn positions values in multi-touch mode
  * @param  pObj Component object pointer
  * @param  pState Multi Touch structure pointer
  * @retval Component status
  */
int32_t GT911_GetMultiTouchState(GT911_Object_t *pObj, GT911_MultiTouch_State_t *pState)
{
  int32_t ret;
  uint8_t  data[40];
  uint32_t i;

  pState->TouchDetected = (uint32_t)GT911_DetectTouch(pObj);

  if(pState->TouchDetected > GT911_MAX_NB_TOUCH)
  {
    ret = GT911_ERROR;
  }
  else if(gt911_read_reg(&pObj->Ctx, GT911_P1_XL_REG, data, (uint16_t)sizeof(data)) != GT911_OK)
  {
    ret = GT911_ERROR;
  }
  else
  {
    ret = GT911_ClearIT(pObj);
    for(i = 0; i < pState->TouchDetected; i++)
    {
      /* Send back first ready X position to caller */
      pState->TouchX[i] = (((uint32_t)data[(i*8U) + 1U] & GT911_P1_XH_TP_BIT_MASK) << 8U) | ((uint32_t)data[(i*6U) + 0U] & GT911_P1_XL_TP_BIT_MASK);
      /* Send back first ready Y position to caller */
      pState->TouchY[i] = (((uint32_t)data[(i*8U) + 3U] & GT911_P1_YH_TP_BIT_MASK) << 8U) | ((uint32_t)data[(i*6U) + 2U] & GT911_P1_YL_TP_BIT_MASK);
      /* Send back first ready Event to caller */
      /* pState->TouchEvent[i] = (((uint32_t)data[i*6U] & GT911_P1_XH_EF_BIT_MASK) >> GT911_P1_XH_EF_BIT_POSITION) */
      /* Send back first ready Weight to caller */
      pState->TouchWeight[i] = (((uint32_t)(data[(i*8U) + 5U] | ((uint32_t)data[(i*6U) + 2U])) & GT911_P1_WEIGHT_BIT_MASK));
      /* Send back first ready Area to caller */
      pState->TouchTrackID[i] = ((uint32_t)data[(i*8U) + 5U] & GT911_P1_TID_BIT_MASK) >> GT911_P1_TID_BIT_POSITION;
    }
    for(i = pState->TouchDetected; i < GT911_MAX_NB_TOUCH; i++)
    {
      // set 0 for non-touch point(s)
      pState->TouchX[i] = 0;
      pState->TouchY[i] = 0;
      pState->TouchWeight[i] = 0;
      pState->TouchTrackID[i] = 0;
    }
  }

  return ret;
}

/**
  * @brief  Get Gesture ID
  * @param  pObj Component object pointer
  * @param  pGestureId Gesture ID pointer
  * @retval Component status
  */
int32_t GT911_GetGesture(GT911_Object_t *pObj, uint8_t *pGestureId)
{
  return gt911_gest_id(&pObj->Ctx, pGestureId);
}

/**
  * @brief  Configure the GT911 device to generate IT on given INT pin
  *         connected to MCU as EXTI.
  * @param  pObj Component object pointer
  * @param  trigger Interrupt trigger
  * @retval Component status
  */
int32_t GT911_EnableIT(GT911_Object_t *pObj)
{
  return gt911_m_sw1(&pObj->Ctx, (0x00 << GT911_M_SW1_BIT_POSITION) & GT911_M_SW1_BIT_MASK);
}

/**
  * @brief  Configure the GT911 device to stop generating IT on the given INT pin
  *         connected to MCU as EXTI.
  * @param  pObj Component object pointer
  * @retval Component status
  */
int32_t GT911_DisableIT(GT911_Object_t *pObj)
{
  return gt911_m_sw1(&pObj->Ctx, GT911_M_SW1_INTERRUPT_FALLING);
}

/**
  * @brief  Get IT status from GT911 interrupt status registers
  *         Should be called Following an EXTI coming to the MCU to know the detailed
  *         reason of the interrupt.
  *         @note : This feature is not applicable to GT911.
  * @param  pObj Component object pointer
  * @retval TS interrupts status : always return GT911_OK here
  */
int32_t GT911_ITStatus(GT911_Object_t *pObj)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  /* Always return GT911_OK as feature not applicable to GT911 */
  return GT911_OK;
}

/**
  * @brief  Clear IT status in GT911 interrupt status clear registers
  *         Should be called Following an EXTI coming to the MCU.
  *         @note : This feature is not applicable to GT911.
  * @param  pObj Component object pointer
  * @retval Component status
  */
int32_t GT911_ClearIT(GT911_Object_t *pObj)
{
  return gt911_clr_int(&pObj->Ctx);
}

/******************** Static functions ****************************************/
#if (GT911_AUTO_CALIBRATION_ENABLED == 1)
/**
  * @brief This function provides accurate delay (in milliseconds)
  * @param pObj Component object pointer
  * @param Delay specifies the delay time length, in milliseconds
  * @retval GT911_OK
  */
static int32_t GT911_Delay(GT911_Object_t *pObj, uint32_t Delay)
{
  uint32_t tickstart;
  tickstart = pObj->IO.GetTick();
  while((pObj->IO.GetTick() - tickstart) < Delay)
  {
  }
  return GT911_OK;
}

/**
  * @brief Start TouchScreen calibration phase
  * @param pObj Component object pointer
  * @retval Status GT911_OK or GT911_ERROR.
  */
static int32_t GT911_TS_Calibration(GT911_Object_t *pObj)
{
  int32_t ret = GT911_OK;
  uint32_t nbr_attempt;
  uint8_t read_data;
  uint8_t end_calibration = 0;

  /* Switch GT911 back to factory mode to calibrate */
  if(gt911_dev_mode_w(&pObj->Ctx, GT911_DEV_MODE_FACTORY) != GT911_OK)
  {
    ret = GT911_ERROR;
  }/* Read back the same register GT911_DEV_MODE_REG */
  else if(gt911_dev_mode_r(&pObj->Ctx, &read_data) != GT911_OK)
  {
    ret = GT911_ERROR;
  }
  else
  {
    (void)GT911_Delay(pObj, 300); /* Wait 300 ms */

    if(read_data != GT911_DEV_MODE_FACTORY )
    {
      /* Return error to caller */
      ret = GT911_ERROR;
    }
    else
    {
      /* Start calibration command */
      read_data= 0x04;
      if(gt911_write_reg(&pObj->Ctx, GT911_TD_STAT_REG, &read_data, 1) != GT911_OK)
      {
        ret = GT911_ERROR;
      }
      else
      {
        (void)GT911_Delay(pObj, 300); /* Wait 300 ms */

        /* 100 attempts to wait switch from factory mode (calibration) to working mode */
        for (nbr_attempt=0; ((nbr_attempt < 100U) && (end_calibration == 0U)) ; nbr_attempt++)
        {
          if(gt911_dev_mode_r(&pObj->Ctx, &read_data) != GT911_OK)
          {
            ret = GT911_ERROR;
            break;
          }
          if(read_data == GT911_DEV_MODE_WORKING)
          {
            /* Auto Switch to GT911_DEV_MODE_WORKING : means calibration have ended */
            end_calibration = 1; /* exit for loop */
          }

          (void)GT911_Delay(pObj, 200); /* Wait 200 ms */
        }
      }
    }
  }

  return ret;
}
#endif /* GT911_AUTO_CALIBRATION_ENABLED == 1 */

/**
  * @brief  Return if there is touches detected or not.
  *         Try to detect new touches and forget the old ones (reset internal global
  *         variables).
  * @param  pObj Component object pointer
  * @retval Number of active touches detected (can be 0, 1~5) or GT911_ERROR
  *         in case of error
  */
static int32_t GT911_DetectTouch(GT911_Object_t *pObj)
{
  int32_t ret;
  uint8_t nb_touch;

  /* Read register GT911_TD_STAT_REG to check number of touches detection */
  if(gt911_td_status(&pObj->Ctx, &nb_touch) != GT911_OK)
  {
    ret = GT911_ERROR;
  }
  else
  {
    if(nb_touch > GT911_MAX_NB_TOUCH)
    {
      /* If invalid number of touch detected, set it to zero */
      ret = 0;
    }
    else
    {
      ret = (int32_t)nb_touch;
    }
  }
  return ret;
}

/**
  * @brief  Function
  * @param  handle: Component object handle
  * @param  Reg The target register address to write
  * @param  pData The target register value to be written
  * @param  Length Buffer size to be read
  * @retval Component status
  */
static int32_t ReadRegWrap(void *handle, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  GT911_Object_t *pObj = (GT911_Object_t *)handle; /* Derogation MISRAC2012-Rule-11.5 */
  return pObj->IO.ReadReg(pObj->IO.Address, Reg, pData, Length);
}

/**
  * @brief  Function
  * @param  handle: Component object handle
  * @param  Reg The target register address to write
  * @param  pData The target register value to be written
  * @param  Length Buffer size to be written
  * @retval Component status
  */
static int32_t WriteRegWrap(void *handle, uint16_t Reg, uint8_t *pData, uint16_t Length)
{
  GT911_Object_t *pObj = (GT911_Object_t *)handle; /* Derogation MISRAC2012-Rule-11.5 */
  return pObj->IO.WriteReg(pObj->IO.Address, Reg, pData, Length);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
