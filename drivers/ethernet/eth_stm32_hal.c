/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME eth_stm32_hal
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <kernel.h>
#include <device.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <errno.h>
#include <stdbool.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <soc.h>
#include <sys/printk.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>

#include "eth.h"
#include "eth_stm32_hal_priv.h"

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    !DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#error DTCM for DMA buffer is activated but zephyr,dtcm is not present in dts
#endif

#if defined(CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER) && \
	    DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
static ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB] __dtcm_noinit_section;
static ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB] __dtcm_noinit_section;
static u8_t dma_rx_buffer[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __dtcm_noinit_section;
static u8_t dma_tx_buffer[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __dtcm_noinit_section;
#else
static ETH_DMADescTypeDef dma_rx_desc_tab[ETH_RXBUFNB] __aligned(4);
static ETH_DMADescTypeDef dma_tx_desc_tab[ETH_TXBUFNB] __aligned(4);
static u8_t dma_rx_buffer[ETH_RXBUFNB][ETH_RX_BUF_SIZE] __aligned(4);
static u8_t dma_tx_buffer[ETH_TXBUFNB][ETH_TX_BUF_SIZE] __aligned(4);
#endif /* CONFIG_ETH_STM32_HAL_USE_DTCM_FOR_DMA_BUFFER */

#if defined(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)
#include <net/can.h>

static void set_mac_to_translator_addr(u8_t *mac_addr)
{
	/* Set the last 14 bit to the translator  link layer address to avoid
	 * address collissions with the 6LoCAN address range
	 */
	mac_addr[4] = (mac_addr[4] & 0xC0) | (NET_CAN_ETH_TRANSLATOR_ADDR >> 8);
	mac_addr[5] = NET_CAN_ETH_TRANSLATOR_ADDR & 0xFF;
}

static void enable_canbus_eth_translator_filter(ETH_HandleTypeDef *heth,
						u8_t *mac_addr)
{
	heth->Instance->MACA1LR = (mac_addr[3] << 24U) | (mac_addr[2] << 16U) |
				  (mac_addr[1] << 8U) | mac_addr[0];
	/*enable filter 1 and ignore byte 5 and 6 for filtering*/
	heth->Instance->MACA1HR = ETH_MACA1HR_AE |  ETH_MACA1HR_MBC_HBits15_8 |
				  ETH_MACA1HR_MBC_HBits7_0;
}
#endif /*CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR*/

static inline void disable_mcast_filter(ETH_HandleTypeDef *heth)
{
	__ASSERT_NO_MSG(heth != NULL);

	u32_t tmp = heth->Instance->MACFFR;

	/* disable multicast filtering */
	tmp &= ~(ETH_MULTICASTFRAMESFILTER_PERFECTHASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_HASHTABLE |
		 ETH_MULTICASTFRAMESFILTER_PERFECT);

	/* enable receiving all multicast frames */
	tmp |= ETH_MULTICASTFRAMESFILTER_NONE;

	heth->Instance->MACFFR = tmp;

	/* Wait until the write operation will be taken into account:
	 * at least four TX_CLK/RX_CLK clock cycles
	 */
	tmp = heth->Instance->MACFFR;
	k_sleep(K_MSEC(1));
	heth->Instance->MACFFR = tmp;
}

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	struct eth_stm32_hal_dev_data *dev_data = DEV_DATA(dev);
	ETH_HandleTypeDef *heth;
	u8_t *dma_buffer;
	int res;
	u16_t total_len;
	__IO ETH_DMADescTypeDef *dma_tx_desc;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	total_len = net_pkt_get_len(pkt);
	if (total_len > ETH_TX_BUF_SIZE) {
		LOG_ERR("PKT to big");
		res = -EIO;
		goto error;
	}

	dma_tx_desc = heth->TxDesc;
	while ((dma_tx_desc->Status & ETH_DMATXDESC_OWN) != (u32_t)RESET) {
		k_yield();
	}

	dma_buffer = (u8_t *)(dma_tx_desc->Buffer1Addr);

	if (net_pkt_read(pkt, dma_buffer, total_len)) {
		res = -EIO;
		goto error;
	}

	if (HAL_ETH_TransmitFrame(heth, total_len) != HAL_OK) {
		LOG_ERR("HAL_ETH_TransmitFrame failed");
		res = -EIO;
		goto error;
	}

	/* When Transmit Underflow flag is set, clear it and issue a
	 * Transmit Poll Demand to resume transmission.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_TUS) != (u32_t)RESET) {
		/* Clear TUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_TUS;
		/* Resume DMA transmission*/
		heth->Instance->DMATPDR = 0;
		res = -EIO;
		goto error;
	}

	res = 0;
error:
	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}

static struct net_if *get_iface(struct eth_stm32_hal_dev_data *ctx,
				u16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(ctx->iface, vlan_tag);
	if (!iface) {
		return ctx->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return ctx->iface;
#endif
}

static struct net_pkt *eth_rx(struct device *dev, u16_t *vlan_tag)
{
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;
	__IO ETH_DMADescTypeDef *dma_rx_desc;
	struct net_pkt *pkt;
	u16_t total_len;
	u8_t *dma_buffer;
	int i;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	if (HAL_ETH_GetReceivedFrame_IT(heth) != HAL_OK) {
		/* no frame available */
		return NULL;
	}

	total_len = heth->RxFrameInfos.length;
	dma_buffer = (u8_t *)heth->RxFrameInfos.buffer;

	pkt = net_pkt_rx_alloc_with_buffer(get_iface(dev_data, *vlan_tag),
					   total_len, AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, dma_buffer, total_len)) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto release_desc;
	}

release_desc:
	/* Release descriptors to DMA */
	/* Point to first descriptor */
	dma_rx_desc = heth->RxFrameInfos.FSRxDesc;
	/* Set Own bit in Rx descriptors: gives the buffers back to DMA */
	for (i = 0; i < heth->RxFrameInfos.SegCount; i++) {
		dma_rx_desc->Status |= ETH_DMARXDESC_OWN;
		dma_rx_desc = (ETH_DMADescTypeDef *)
			(dma_rx_desc->Buffer2NextDescAddr);
	}

	/* Clear Segment_Count */
	heth->RxFrameInfos.SegCount = 0;

	/* When Rx Buffer unavailable flag is set: clear it
	 * and resume reception.
	 */
	if ((heth->Instance->DMASR & ETH_DMASR_RBUS) != (u32_t)RESET) {
		/* Clear RBUS ETHERNET DMA flag */
		heth->Instance->DMASR = ETH_DMASR_RBUS;
		/* Resume DMA reception */
		heth->Instance->DMARPDR = 0;
	}

#if defined(CONFIG_NET_VLAN)
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
		struct net_eth_vlan_hdr *hdr_vlan =
			(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

		net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
		*vlan_tag = net_pkt_vlan_tag(pkt);

#if CONFIG_NET_TC_RX_COUNT > 1
		enum net_priority prio;

		prio = net_vlan2priority(net_pkt_vlan_priority(pkt));
		net_pkt_set_priority(pkt, prio);
#endif
	} else {
		net_pkt_set_iface(pkt, dev_data->iface);
	}
#endif /* CONFIG_NET_VLAN */

	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(dev_data, *vlan_tag));
	}

	return pkt;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;
	struct net_pkt *pkt;
	int res;
	u32_t status;

	__ASSERT_NO_MSG(arg1 != NULL);
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	dev = (struct device *)arg1;
	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	while (1) {
		res = k_sem_take(&dev_data->rx_int_sem,
			K_MSEC(CONFIG_ETH_STM32_CARRIER_CHECK_RX_IDLE_TIMEOUT_MS));
		if (res == 0) {
			/* semaphore taken, update link status and receive packets */
			if (dev_data->link_up != true) {
				dev_data->link_up = true;
				net_eth_carrier_on(get_iface(dev_data,
							     vlan_tag));
			}
			while ((pkt = eth_rx(dev, &vlan_tag)) != NULL) {
				res = net_recv_data(net_pkt_iface(pkt), pkt);
				if (res < 0) {
					eth_stats_update_errors_rx(
							net_pkt_iface(pkt));
					LOG_ERR("Failed to enqueue frame "
						"into RX queue: %d", res);
					net_pkt_unref(pkt);
				}
			}
		} else if (res == -EAGAIN) {
#if CONFIG_ETH_MAC2MAC
			if (dev_data->link_up != true) {
				dev_data->link_up = true;
				net_eth_carrier_on(get_iface(dev_data,
							     vlan_tag));
			}
#else
			/* semaphore timeout period expired, check link status */
			if (HAL_ETH_ReadPHYRegister(&dev_data->heth, PHY_BSR,
				(uint32_t *) &status) == HAL_OK) {
				if ((status & PHY_LINKED_STATUS) == PHY_LINKED_STATUS) {
					if (dev_data->link_up != true) {
						dev_data->link_up = true;
						net_eth_carrier_on(
							get_iface(dev_data,
								  vlan_tag));
					}
				} else {
					if (dev_data->link_up != false) {
						dev_data->link_up = false;
						net_eth_carrier_off(
							get_iface(dev_data,
								  vlan_tag));
					}
				}
			}
#endif
		}
	}
}

static void eth_isr(void *arg)
{
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;

	__ASSERT_NO_MSG(arg != NULL);

	dev = (struct device *)arg;
	dev_data = DEV_DATA(dev);

	__ASSERT_NO_MSG(dev_data != NULL);

	heth = &dev_data->heth;

	__ASSERT_NO_MSG(heth != NULL);

	HAL_ETH_IRQHandler(heth);
}


void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth_handle)
{
	__ASSERT_NO_MSG(heth_handle != NULL);

	struct eth_stm32_hal_dev_data *dev_data =
		CONTAINER_OF(heth_handle, struct eth_stm32_hal_dev_data, heth);

	__ASSERT_NO_MSG(dev_data != NULL);

	k_sem_give(&dev_data->rx_int_sem);
}

#if defined(CONFIG_ETH_STM32_HAL_RANDOM_MAC)
static void generate_mac(u8_t *mac_addr)
{
	gen_random_mac(mac_addr, ST_OUI_B0, ST_OUI_B1, ST_OUI_B2);
}
#endif

static int eth_initialize(struct device *dev)
{
	struct eth_stm32_hal_dev_data *dev_data;
	const struct eth_stm32_hal_dev_cfg *cfg;
	ETH_HandleTypeDef *heth;
	u8_t hal_ret;
	int ret = 0;

	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);
	cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(dev_data != NULL);
	__ASSERT_NO_MSG(cfg != NULL);

	dev_data->clock = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(dev_data->clock != NULL);

	/* enable clock */
	ret = clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken);
	ret |= clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken_tx);
	ret |= clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken_rx);
	ret |= clock_control_on(dev_data->clock,
		(clock_control_subsys_t *)&cfg->pclken_ptp);

	if (ret) {
		LOG_ERR("Failed to enable ethernet clock");
		return -EIO;
	}

	heth = &dev_data->heth;

#if defined(CONFIG_ETH_STM32_HAL_RANDOM_MAC)
	generate_mac(dev_data->mac_addr);
#endif
#if defined(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)
	set_mac_to_translator_addr(dev_data->mac_addr);
#endif

	heth->Init.MACAddr = dev_data->mac_addr;

	hal_ret = HAL_ETH_Init(heth);
	if (hal_ret == HAL_TIMEOUT) {
		/* HAL Init time out. This could be linked to */
		/* a recoverable error. Log the issue and continue */
		/* driver initialisation */
		LOG_ERR("HAL_ETH_Init Timed out");
	} else if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_ETH_Init failed: %d", hal_ret);
		return -EINVAL;
	}

	dev_data->link_up = false;

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, UINT_MAX);

	/* Start interruption-poll thread */
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_THREAD_STACK_SIZEOF(dev_data->rx_thread_stack),
			rx_thread, (void *) dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_STM32_HAL_RX_THREAD_PRIO),
			0, K_NO_WAIT);

	HAL_ETH_DMATxDescListInit(heth, dma_tx_desc_tab,
		&dma_tx_buffer[0][0], ETH_TXBUFNB);
	HAL_ETH_DMARxDescListInit(heth, dma_rx_desc_tab,
		&dma_rx_buffer[0][0], ETH_RXBUFNB);

	HAL_ETH_Start(heth);

	disable_mcast_filter(heth);

#if defined(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)
	enable_canbus_eth_translator_filter(heth, dev_data->mac_addr);
#endif

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	struct device *dev;
	struct eth_stm32_hal_dev_data *dev_data;

	__ASSERT_NO_MSG(iface != NULL);

	dev = net_if_get_device(iface);
	__ASSERT_NO_MSG(dev != NULL);

	dev_data = DEV_DATA(dev);
	__ASSERT_NO_MSG(dev_data != NULL);

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (dev_data->iface == NULL) {
		dev_data->iface = iface;

		/* Now that the iface is setup, we are safe to enable IRQs. */
		__ASSERT_NO_MSG(DEV_CFG(dev)->config_func != NULL);
		DEV_CFG(dev)->config_func();
	}

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

static enum ethernet_hw_caps eth_stm32_hal_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
		;
}

static int eth_stm32_hal_set_config(struct device *dev,
				    enum ethernet_config_type type,
				    const struct ethernet_config *config)
{
	struct eth_stm32_hal_dev_data *dev_data;
	ETH_HandleTypeDef *heth;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		dev_data = DEV_DATA(dev);
		heth = &dev_data->heth;

		memcpy(dev_data->mac_addr, config->mac_address.addr, 6);
		heth->Instance->MACA0HR = (dev_data->mac_addr[5] << 8) |
			dev_data->mac_addr[4];
		heth->Instance->MACA0LR = (dev_data->mac_addr[3] << 24) |
			(dev_data->mac_addr[2] << 16) |
			(dev_data->mac_addr[1] << 8) |
			dev_data->mac_addr[0];
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		dev_data = DEV_DATA(dev);
		heth = &dev_data->heth;

		if(config->promisc_mode) {
			heth->Instance->MACFFR |= ETH_MACFFR_PM;
		} else {
			heth->Instance->MACFFR &= ~ETH_MACFFR_PM;
		}
		return 0;
#endif
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_stm32_hal_get_capabilities,
	.set_config = eth_stm32_hal_set_config,
	.send = eth_tx,
};

static struct device DEVICE_NAME_GET(eth0_stm32_hal);

static void eth0_irq_config(void)
{
	IRQ_CONNECT(ETH_IRQn, CONFIG_ETH_STM32_HAL_IRQ_PRI, eth_isr,
		    DEVICE_GET(eth0_stm32_hal), 0);
	irq_enable(ETH_IRQn);
}

static const struct eth_stm32_hal_dev_cfg eth0_config = {
	.config_func = eth0_irq_config,
	.pclken   =   { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMAC },
	.pclken_tx =  { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMACTX },
	.pclken_rx =  { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMACRX },
	.pclken_ptp = { .bus = STM32_CLOCK_BUS_AHB1,
			.enr = LL_AHB1_GRP1_PERIPH_ETHMACPTP },
};

static struct eth_stm32_hal_dev_data eth0_data = {
	.heth = {
		.Instance = ETH,
		.Init = {
			.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE,
			.PhyAddress = CONFIG_ETH_STM32_HAL_PHY_ADDRESS,
			.RxMode = ETH_RXINTERRUPT_MODE,
			.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE,
#if defined(CONFIG_ETH_STM32_HAL_MII)
			.MediaInterface = ETH_MEDIA_INTERFACE_MII,
#else
			.MediaInterface = ETH_MEDIA_INTERFACE_RMII,
#endif
		},
	},
	.mac_addr = {
		ST_OUI_B0,
		ST_OUI_B1,
		ST_OUI_B2,
#if !defined(CONFIG_ETH_STM32_HAL_RANDOM_MAC)
		CONFIG_ETH_STM32_HAL_MAC3,
		CONFIG_ETH_STM32_HAL_MAC4,
		CONFIG_ETH_STM32_HAL_MAC5
#endif
	},
};

ETH_NET_DEVICE_INIT(eth0_stm32_hal, CONFIG_ETH_STM32_HAL_NAME, eth_initialize,
		    device_pm_control_nop, &eth0_data, &eth0_config,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_STM32_HAL_MTU);
