/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for OLIMEX-STM32-E407 board */
static const struct pin_config pinconf[] = {
#if DT_HAS_NODE(DT_NODELABEL(usart1))
	{STM32_PIN_PB6, STM32F4_PINMUX_FUNC_PB6_USART1_TX},
	{STM32_PIN_PB7, STM32F4_PINMUX_FUNC_PB7_USART1_RX},
#endif
#if DT_HAS_NODE(DT_NODELABEL(usart3))
	{STM32_PIN_PB10, STM32F4_PINMUX_FUNC_PB10_USART3_TX},
	{STM32_PIN_PB11, STM32F4_PINMUX_FUNC_PB11_USART3_RX},
#endif
#if DT_HAS_NODE(DT_NODELABEL(usart6))
	{STM32_PIN_PC6, STM32F4_PINMUX_FUNC_PC6_USART6_TX},
	{STM32_PIN_PC7, STM32F4_PINMUX_FUNC_PC7_USART6_RX},
#endif
#ifdef CONFIG_ETH_STM32_HAL
	{STM32_PIN_PC1, STM32F4_PINMUX_FUNC_PC1_ETH},
	{STM32_PIN_PC4, STM32F4_PINMUX_FUNC_PC4_ETH},
	{STM32_PIN_PC5, STM32F4_PINMUX_FUNC_PC5_ETH},

	{STM32_PIN_PA1, STM32F4_PINMUX_FUNC_PA1_ETH},
	{STM32_PIN_PA2, STM32F4_PINMUX_FUNC_PA2_ETH},
	{STM32_PIN_PA3, STM32F4_PINMUX_FUNC_PA3_ETH},
	{STM32_PIN_PA7, STM32F4_PINMUX_FUNC_PA7_ETH},

	{STM32_PIN_PG11, STM32F4_PINMUX_FUNC_PG11_ETH},
	{STM32_PIN_PG13, STM32F4_PINMUX_FUNC_PG13_ETH},
	{STM32_PIN_PG14, STM32F4_PINMUX_FUNC_PG14_ETH},
#endif /* CONFIG_ETH_STM32_HAL */
#ifdef CONFIG_USB_DC_STM32
#if DT_HAS_NODE(DT_INST(0, st_stm32_otgfs))
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif /* DT_HAS_NODE(DT_INST(0, st_stm32_otgfs)) */
#if DT_HAS_NODE(DT_INST(0, st_stm32_otghs))
	{STM32_PIN_PB14, STM32F4_PINMUX_FUNC_PB14_OTG_HS_DM},
	{STM32_PIN_PB15, STM32F4_PINMUX_FUNC_PB15_OTG_HS_DP},
#endif /* DT_HAS_NODE(DT_INST(0, st_stm32_otghs)) */
#endif  /* CONFIG_USB_DC_STM32 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
