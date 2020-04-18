/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_ADC_0_NAME			DT_LABEL(DT_INST(0, st_stm32_adc))
#define DT_ADC_1_NAME			DT_LABEL(DT_INST(1, st_stm32_adc))

#define DT_FLASH_DEV_NAME			DT_LABEL(DT_INST(0, st_stm32g4_flash_controller))

#define DT_RTC_0_NAME				DT_LABEL(DT_INST(0, st_stm32_rtc))

/* End of SoC Level DTS fixup file */
