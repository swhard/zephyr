/* SPDX-License-Identifier: Apache-2.0 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_I2C_1_BASE_ADDRESS		DT_ST_STM32_I2C_V1_40005400_BASE_ADDRESS
#define DT_I2C_1_EVENT_IRQ_PRI		DT_ST_STM32_I2C_V1_40005400_IRQ_EVENT_PRIORITY
#define DT_I2C_1_ERROR_IRQ_PRI		DT_ST_STM32_I2C_V1_40005400_IRQ_ERROR_PRIORITY
#define DT_I2C_1_EVENT_IRQ		DT_ST_STM32_I2C_V1_40005400_IRQ_EVENT
#define DT_I2C_1_ERROR_IRQ		DT_ST_STM32_I2C_V1_40005400_IRQ_ERROR
#define DT_I2C_1_BITRATE		DT_ST_STM32_I2C_V1_40005400_CLOCK_FREQUENCY
#define DT_I2C_1_CLOCK_BITS		DT_ST_STM32_I2C_V1_40005400_CLOCK_BITS
#define DT_I2C_1_CLOCK_BUS		DT_ST_STM32_I2C_V1_40005400_CLOCK_BUS

#define DT_I2C_2_BASE_ADDRESS		DT_ST_STM32_I2C_V1_40005800_BASE_ADDRESS
#define DT_I2C_2_EVENT_IRQ_PRI		DT_ST_STM32_I2C_V1_40005800_IRQ_EVENT_PRIORITY
#define DT_I2C_2_ERROR_IRQ_PRI		DT_ST_STM32_I2C_V1_40005800_IRQ_ERROR_PRIORITY
#define DT_I2C_2_EVENT_IRQ		DT_ST_STM32_I2C_V1_40005800_IRQ_EVENT
#define DT_I2C_2_ERROR_IRQ		DT_ST_STM32_I2C_V1_40005800_IRQ_ERROR
#define DT_I2C_2_BITRATE		DT_ST_STM32_I2C_V1_40005800_CLOCK_FREQUENCY
#define DT_I2C_2_CLOCK_BITS		DT_ST_STM32_I2C_V1_40005800_CLOCK_BITS
#define DT_I2C_2_CLOCK_BUS		DT_ST_STM32_I2C_V1_40005800_CLOCK_BUS

#define DT_WDT_0_NAME			DT_LABEL(DT_INST(0, st_stm32_watchdog))

#define DT_ADC_1_NAME			DT_ST_STM32_ADC_40012400_LABEL

#define DT_CAN_1_BASE_ADDRESS		DT_ST_STM32_CAN_40006400_BASE_ADDRESS
#define DT_CAN_1_BUS_SPEED		DT_ST_STM32_CAN_40006400_BUS_SPEED
#define DT_CAN_1_NAME			DT_ST_STM32_CAN_40006400_LABEL
#define DT_CAN_1_IRQ_TX			DT_ST_STM32_CAN_40006400_IRQ_TX
#define DT_CAN_1_IRQ_RX0		DT_ST_STM32_CAN_40006400_IRQ_RX0
#define DT_CAN_1_IRQ_RX1		DT_ST_STM32_CAN_40006400_IRQ_RX1
#define DT_CAN_1_IRQ_SCE		DT_ST_STM32_CAN_40006400_IRQ_SCE
#define DT_CAN_1_IRQ_PRIORITY		DT_ST_STM32_CAN_40006400_IRQ_0_PRIORITY
#define DT_CAN_1_SJW			DT_ST_STM32_CAN_40006400_SJW
#define DT_CAN_1_PROP_SEG		DT_ST_STM32_CAN_40006400_PROP_SEG
#define DT_CAN_1_PHASE_SEG1		DT_ST_STM32_CAN_40006400_PHASE_SEG1
#define DT_CAN_1_PHASE_SEG2		DT_ST_STM32_CAN_40006400_PHASE_SEG2
#define DT_CAN_1_CLOCK_BUS		DT_ST_STM32_CAN_40006400_CLOCK_BUS
#define DT_CAN_1_CLOCK_BITS		DT_ST_STM32_CAN_40006400_CLOCK_BITS

#define DT_FLASH_DEV_NAME		DT_LABEL(DT_INST(0, st_stm32f1_flash_controller))

/* End of SoC Level DTS fixup file */
