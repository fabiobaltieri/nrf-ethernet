#ifndef _HALCONF_H_
#define _HALCONF_H_

#include "mcuconf.h"

#define HAL_USE_TM                  FALSE
#define HAL_USE_PAL                 TRUE
#define HAL_USE_ADC                 FALSE
#define HAL_USE_CAN                 FALSE
#define HAL_USE_EXT                 FALSE
#define HAL_USE_GPT                 FALSE
#define HAL_USE_I2C                 FALSE
#define HAL_USE_ICU                 FALSE
#define HAL_USE_MAC                 FALSE
#define HAL_USE_MMC_SPI             FALSE
#define HAL_USE_PWM                 FALSE
#define HAL_USE_RTC                 FALSE
#define HAL_USE_SDC                 FALSE
#define HAL_USE_SERIAL              TRUE
#define HAL_USE_SERIAL_USB          FALSE
#define HAL_USE_SPI                 FALSE
#define HAL_USE_UART                FALSE
#define HAL_USE_USB                 FALSE

#define ADC_USE_WAIT                TRUE
#define ADC_USE_MUTUAL_EXCLUSION    TRUE

#define CAN_USE_SLEEP_MODE          TRUE

#define I2C_USE_MUTUAL_EXCLUSION    TRUE

#define MAC_USE_ZERO_COPY           FALSE
#define MAC_USE_EVENTS              TRUE

#define MMC_NICE_WAITING            TRUE

#define SDC_INIT_RETRY              100
#define SDC_MMC_SUPPORT             FALSE
#define SDC_NICE_WAITING            TRUE

#define SERIAL_DEFAULT_BITRATE      115200
#define SERIAL_BUFFERS_SIZE         16

#define SPI_USE_WAIT                TRUE
#define SPI_USE_MUTUAL_EXCLUSION    TRUE

#endif /* _HALCONF_H_ */
