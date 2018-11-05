/* linux/drivers/i2c/busses/i2c-hisilicon.c
 *
 * HISILICON I2C Controller
 *
 * Copyright (c) 2014 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <mach/io.h>

#include "i2c-hisilicon.h"

#ifdef CONFIG_HI_DMAC
#include <linux/hidmac.h>
#endif

#define HI_I2C	"hisi_i2c"

#define hi_err(x...) \
	do { \
		pr_alert("%s->%d: ", __func__, __LINE__); \
		pr_alert(x); \
		pr_alert("\n"); \
	} while (0)

/* #define HI_I2C_DEBUG */

#ifdef HI_I2C_DEBUG

#define hi_msg(x...) \
	do { \
		pr_alert("%s (line:%d) ", __func__, __LINE__); \
		pr_alert(x); \
	} while (0)
#else
#define hi_msg(args...) do { } while (0)
#endif

#define I2C_WAIT_TIME_OUT	20000

#define I2C_DFT_RATE	(100000)

struct hi_i2c {
	unsigned char __iomem *regbase;
	struct device *dev;
	struct resource *mem;
	unsigned int irq;
	struct i2c_adapter adap;
	struct i2c_msg *msgs;
	__u16 msg_num;
	__u16 msg_addr;
	unsigned int msg_index;
	struct hi_platform_i2c *pdata;
	unsigned int g_last_dev_addr;
	unsigned int g_last_mode;
};

unsigned int get_apb_clk(void);

static int hi_i2c_abortprocess(struct hi_i2c *pinfo)
{
	unsigned int auto_status;
	unsigned int tx_src;

	tx_src = readl(pinfo->regbase + I2C_TX_ABRT_SRC);
	hi_err("tx_abrt_src is %x.\n", tx_src);

	auto_status = readl(pinfo->regbase + I2C_AUTO_REG);

	/* clear 0xB0 err status */
	/* auto_mst_tx_abrt_clr
	   auto_tx_cmd_fifo_over_clr
	   auto_rx_cmd_fifo_under_clr
	   auto_rx_cmd_fifo_over_clr
	 */
	auto_status |= 0x0f000000;
	writel(auto_status, pinfo->regbase + I2C_AUTO_REG);
	writel(0x1, pinfo->regbase + I2C_CLR_INTR_REG);

	/* disable i2c */
	writel(0, pinfo->regbase + I2C_ENABLE_REG);

	/* enable i2c */
	writel(0x1, pinfo->regbase + I2C_ENABLE_REG);

	return 0;
}

void hi_i2c_set_rate(struct hi_i2c *pinfo)
{
	unsigned int apb_clk, scl_h, scl_l, hold;

	/* get apb bus clk for diff plat */
	apb_clk = get_apb_clk();

	/* set SCLH and SCLL depend on apb_clk and def_rate */
	if (pinfo->pdata->clk_limit <= I2C_DFT_RATE) {
		/* in normal mode		F_scl: def_rate
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.5
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.5
		*/
		scl_h = (apb_clk / I2C_DFT_RATE) / 2;
		scl_l = scl_h;
	} else {
		/* in fast mode		F_scl: def_rate
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.36
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.64
		*/
		scl_h = ((apb_clk / 100) * 36) / pinfo->pdata->clk_limit;
		scl_l = ((apb_clk / 100) * 64) / pinfo->pdata->clk_limit;
	}

	writel(scl_h, pinfo->regbase + I2C_SCL_H_REG);
	writel(scl_l, pinfo->regbase + I2C_SCL_L_REG);

	/* set hi_i2c hold time */
	hold = scl_h / 2;
	writel(hold, pinfo->regbase + I2C_SDA_HOLD_REG);
}

void hi_i2c_hw_init(struct hi_i2c *pinfo)
{
	unsigned int temp, rx_fifo, tx_fifo;

	/* unlock hi_i2c controller to access */
	writel(HI_I2C_UNLOCK_VALUE, pinfo->regbase + I2C_LOCK_REG);

	/* disable hi_i2c controller */
	temp = readl(pinfo->regbase + I2C_ENABLE_REG);
	writel((temp & ~HI_I2C_ENABLE), pinfo->regbase + I2C_ENABLE_REG);

	/* disable hi_i2c auto_mode */
	writel(HI_I2C_AUTO_MODE_OFF, pinfo->regbase + I2C_AUTO_REG);

	/* set hi_i2c in fast mode */
	writel(HI_I2C_FAST_MODE, pinfo->regbase + I2C_CON_REG);

	/* set hi_i2c rate */
	hi_i2c_set_rate(pinfo);

	rx_fifo = CONFIG_HI_I2C_RX_FIFO;
	tx_fifo = CONFIG_HI_I2C_TX_FIFO;

	/* set hi_i2c fifo */
	writel(rx_fifo, pinfo->regbase + I2C_RX_TL_REG);
	writel(tx_fifo, pinfo->regbase + I2C_TX_TL_REG);

	/* enable interrupt mask */
	writel(DISABLE_ALL_INTERRUPTS, pinfo->regbase + I2C_INTR_MASK_REG);

	/* enable hi_i2c controller */
	temp = readl(pinfo->regbase + I2C_ENABLE_REG);
	writel((temp | HI_I2C_ENABLE), pinfo->regbase + I2C_ENABLE_REG);

	pinfo->g_last_dev_addr = 0;
	pinfo->g_last_mode = I2C_MODE_NONE;

	pinfo->msgs = NULL;
	pinfo->msg_num = 0;
}

int hi_i2c_wait_idle(struct hi_i2c *pinfo)
{
	unsigned int val;
	unsigned int time_cnt;

	time_cnt = 0;
	do {
		val = readl(pinfo->regbase + I2C_INTR_RAW_REG);
		if (val & I2C_RAW_TX_ABORT) {
			hi_err("wait last i2c fifo is empty abort! "\
					"int_raw_status: %#x!\n", val);
			return hi_i2c_abortprocess(pinfo);
		}

		val = readl(pinfo->regbase + I2C_AUTO_REG);
		if (!IS_RX_FIFO_EMPTY(val))
			readl(pinfo->regbase + I2C_TX_RX_REG);

		if (IS_FIFO_EMPTY(val))
			break;

		if (time_cnt > I2C_WAIT_TIME_OUT) {
			hi_err("wait last i2c fifo is empty timeout! "\
					"auto_status: %#x\n", val);
			return -EBUSY;
		}
		time_cnt++;
		udelay(50);
	} while (1);

	udelay(10);

	time_cnt = 0;
	do {
		val = readl(pinfo->regbase + I2C_INTR_RAW_REG);
		if (val & I2C_RAW_TX_ABORT) {
			hi_err("wait last i2c is idle abort! "\
					"int_raw_status: %#x!\n", val);
			return hi_i2c_abortprocess(pinfo);
		}

		val = readl(pinfo->regbase + I2C_STATUS_REG);
		if (IS_I2C_IDLE(val))
			break;

		if (time_cnt > I2C_WAIT_TIME_OUT) {
			hi_err("wait last i2c is idle timeout! "\
					"auto_status: %#x\n", val);
			return -EBUSY;
		}
		time_cnt++;
		udelay(50);
	} while (1);

	return 0;
}

/* wait until tx fifo is not full */
int hi_i2c_wait_txfifo_notfull(struct hi_i2c *pinfo)
{
	unsigned int val;
	unsigned int time_cnt;

	time_cnt = 0;
	do {
		val = readl(pinfo->regbase + I2C_INTR_RAW_REG);
		if (val & I2C_RAW_TX_ABORT) {
			hi_err("abort! last int_raw_status: %#x!\n", val);
			return hi_i2c_abortprocess(pinfo);
		}

		val = readl(pinfo->regbase + I2C_AUTO_REG);
		if (!IS_RX_FIFO_EMPTY(val))
			readl(pinfo->regbase + I2C_TX_RX_REG);

		if (val & I2c_AUTO_TX_FIFO_NOT_FULL)
			break;

		if (time_cnt > I2C_WAIT_TIME_OUT) {
			hi_err("timeout! last auto_status: %#x\n", val);
			return -EBUSY;
		}
		time_cnt++;
		udelay(50);
	} while (1);

	return 0;
}

/* wait until tx fifo is not empty */
int hi_i2c_wait_rxfifo_notempty(struct hi_i2c *pinfo)
{
	unsigned int val;
	unsigned int time_cnt;

	time_cnt = 0;
	do {
		val = readl(pinfo->regbase + I2C_INTR_RAW_REG);
		if ((val & I2C_RAW_TX_ABORT) == I2C_RAW_TX_ABORT) {
			hi_err("abort! int_raw_status: %#x!\n", val);
			hi_i2c_abortprocess(pinfo);
			return -EIO;
		}

		val = readl(pinfo->regbase + I2C_AUTO_REG);
		if (!IS_RX_FIFO_EMPTY(val))
			break;

		if (time_cnt > I2C_WAIT_TIME_OUT) {
			hi_err("timeout! auto_status: %#x\n", val);
			hi_i2c_abortprocess(pinfo);
			return -EBUSY;
		}
		time_cnt++;
		udelay(50);
	} while (1);

	return 0;
}

static inline int hi_i2c_set_dev_addr_and_mode(struct hi_i2c *pinfo,
		unsigned int work_mode)
{
	unsigned int dev_addr = pinfo->msgs->addr;

	if ((pinfo->g_last_dev_addr == dev_addr)
			&& (pinfo->g_last_mode == work_mode))
		return 0;

	/* wait until all cmd in fifo is finished and i2c is idle */
	if (hi_i2c_wait_idle(pinfo) < 0)
		return -1;

	/* disable i2c */
	writel(0x0, pinfo->regbase + I2C_ENABLE_REG);
	/* clear interrupt */
	writel(0x1, pinfo->regbase + I2C_CLR_INTR_REG);
	/* enable interrupt mask */
	writel(DISABLE_ALL_INTERRUPTS, pinfo->regbase + I2C_INTR_MASK_REG);
	/* clear err status */
	writel(0x0f000000, pinfo->regbase + I2C_AUTO_REG);

	/* different device, need to reinit i2c ctrl */
	if ((pinfo->g_last_dev_addr) != dev_addr) {
		/* set slave dev addr */
		writel((dev_addr & 0xff)>>1, pinfo->regbase + I2C_TAR_REG);
		pinfo->g_last_dev_addr = dev_addr;
	}

	if (pinfo->g_last_mode != work_mode) {

		/* set auto mode */
		if (work_mode == I2C_MODE_AUTO) {
			writel(0x0, pinfo->regbase + I2C_DMA_CMD0);
			writel(0x80000000, pinfo->regbase + I2C_AUTO_REG);
			pinfo->g_last_mode = work_mode;
		} else if (work_mode == I2C_MODE_DMA) {
			writel(0x0, pinfo->regbase + I2C_AUTO_REG);
			pinfo->g_last_mode = work_mode;
		} else {
			hi_err("invalid i2c mode\n");
			return -1;
		}
	}

	/*  enable i2c */
	writel(0x1, pinfo->regbase + I2C_ENABLE_REG);

	hi_msg("\n@@@@@@@@@@\n");

	return 0;
}

int hi_i2c_write(struct hi_i2c *pinfo)
{
	unsigned int reg_val;
	unsigned int temp_reg;
	unsigned int temp_data;
	unsigned int temp_auto_reg;
        unsigned int min_msgs_len = 0;
	struct i2c_msg *msgs = pinfo->msgs;

	min_msgs_len = (msgs->flags & I2C_M_16BIT_REG) ? 2 : 1;
	min_msgs_len += (msgs->flags & I2C_M_16BIT_DATA) ? 2 : 1;
	if (msgs->len < min_msgs_len){
		hi_err("Unsupported this length: %d!\n", msgs->len);
		return -1;
	}

	if (hi_i2c_set_dev_addr_and_mode(pinfo, I2C_MODE_AUTO) < 0)
		return -1;

	temp_auto_reg = HI_I2C_WRITE;

	if (msgs->flags & I2C_M_16BIT_REG) {
		/* 16bit reg addr */
		temp_auto_reg |= I2C_AUTO_ADDR;

		/* switch high byte and low byte */
		temp_reg = msgs->buf[pinfo->msg_index] << 8;

		pinfo->msg_index++;

		temp_reg |= msgs->buf[pinfo->msg_index];

		pinfo->msg_index++;
	} else {
		temp_reg = msgs->buf[pinfo->msg_index];
		pinfo->msg_index++;
	}

	if (msgs->flags & I2C_M_16BIT_DATA) {
		/* 16bit data */
		temp_auto_reg |= I2C_AUTO_DATA;

		/* switch high byte and low byte */
		temp_data =  msgs->buf[pinfo->msg_index] << 8;

		pinfo->msg_index++;

		temp_data |= msgs->buf[pinfo->msg_index];

		pinfo->msg_index++;
	} else {
		temp_data = msgs->buf[pinfo->msg_index];
		pinfo->msg_index++;
	}

	writel(temp_auto_reg, pinfo->regbase + I2C_AUTO_REG);
	hi_msg("temp_auto_reg: 0x%x\n", temp_auto_reg);

	/* set write reg&data */
	reg_val = (temp_reg << REG_SHIFT) | temp_data;

	/* wait until tx fifo not full */
	if (hi_i2c_wait_txfifo_notfull(pinfo) < 0)
		return -1;

	hi_msg("reg_val = %x\n", reg_val);

	writel(reg_val, pinfo->regbase + I2C_TX_RX_REG);

	hi_msg("dev_addr =%x, reg_addr = %x, Data = %x\n",
		pinfo->msgs->addr, pinfo->msgs->buf[0], pinfo->msgs->buf[1]);

	return pinfo->msg_index;
}

unsigned int hi_i2c_read(struct hi_i2c *pinfo)
{
	unsigned int reg_val;
	unsigned int temp_reg;
	unsigned int ret_data = 0xffff;
	unsigned int temp_auto_reg;
	unsigned int data_num = 0;
        unsigned int min_msgs_len = 0;
	struct i2c_msg *msgs = pinfo->msgs;

	min_msgs_len = (msgs->flags & I2C_M_16BIT_REG) ? 2 : 1;
	if (msgs->len < min_msgs_len){
		hi_err("Unsupported this length: %d!\n", msgs->len);
		return -1;
	}

	if (hi_i2c_set_dev_addr_and_mode(pinfo, I2C_MODE_AUTO) < 0)
		return -1;

	temp_auto_reg = HI_I2C_READ;

	if (msgs->flags & I2C_M_16BIT_REG) {
		/* 16bit reg addr */
		temp_auto_reg |= I2C_AUTO_ADDR;

		/* switch high byte and low byte */
		temp_reg = msgs->buf[pinfo->msg_index] << 8;
		pinfo->msg_index++;
		temp_reg |= msgs->buf[pinfo->msg_index];
	} else {
		temp_reg = msgs->buf[pinfo->msg_index];
		pinfo->msg_index++;
	}

	if (msgs->flags & I2C_M_16BIT_DATA)
		/* 16bit data */
		temp_auto_reg |= I2C_AUTO_DATA;

	writel(temp_auto_reg, pinfo->regbase + I2C_AUTO_REG);
	hi_msg("temp_auto_reg: 0x%x\n", temp_auto_reg);

	/* 1. write addr */
	reg_val = temp_reg << REG_SHIFT;
	hi_msg("reg_val %x\n", reg_val);

	/* wait until tx fifo not full  */
	if (hi_i2c_wait_txfifo_notfull(pinfo) < 0)
		return -1;

	/* regaddr */
	writel(reg_val, pinfo->regbase + I2C_TX_RX_REG);

	/* 2. read return data */
	/* wait until rx fifo not empty  */
	if (hi_i2c_wait_rxfifo_notempty(pinfo) < 0)
		return -1;

	ret_data = readl(pinfo->regbase + I2C_TX_RX_REG) & DATA_16BIT_MASK;
	hi_msg("ret_data = %x\n", ret_data);

	if (msgs->flags & I2C_M_16BIT_DATA) {
		pinfo->msgs->buf[0] = ret_data & DATA_8BIT_MASK;
		pinfo->msgs->buf[1] = (ret_data >> 8) & DATA_8BIT_MASK;
		data_num = 2;
	} else {
		pinfo->msgs->buf[0] = ret_data & DATA_8BIT_MASK;
		data_num = 1;
	}

	writel(0x1, pinfo->regbase + I2C_CLR_INTR_REG);

	return data_num;
}

static int hi_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
		int num)
{
	struct hi_i2c *pinfo;
	int errorcode;

	pinfo = (struct hi_i2c *)i2c_get_adapdata(adap);

	if (num <= 0){
	    errorcode = -EIO;
            goto end;
        }
	pinfo->msgs = msgs;
	pinfo->msg_num = num;
	pinfo->msg_index = 0;

	if (msgs->flags & I2C_M_RD)
		errorcode = hi_i2c_read(pinfo);
	else
		errorcode = hi_i2c_write(pinfo);
end:
	return errorcode;
}

/************************************
	* dma functions *
************************************/
#ifdef CONFIG_HI_DMAC
void hi_i2c_dma_start(struct hi_i2c *pinfo, unsigned int dir)
{
	writel((1 << dir), pinfo->regbase + I2C_DMA_CTRL_REG);
}

void hi_i2c_dmac_config(struct hi_i2c *pinfo, unsigned int dir)
{
	/* 1. enable RX(0) or TX(1) in DMA mode */
	hi_i2c_dma_start(pinfo, dir);

	/* 2. set dma fifo */
	writel(4, pinfo->regbase + I2C_DMA_TDLR);
	writel(4, pinfo->regbase + I2C_DMA_RDLR);
}

void hi_i2c_start_rx(struct hi_i2c *pinfo, unsigned int reg_addr,
			unsigned int length)
{
	unsigned int reg;

	writel(reg_addr, pinfo->regbase + I2C_DMA_CMD1);
	writel(length, pinfo->regbase + I2C_DMA_CMD2);

	reg = readl(pinfo->regbase + I2C_DMA_CMD0);

	/*start tx*/
	reg &= ~0x40000000;
	writel((0x80000000 | reg), pinfo->regbase + I2C_DMA_CMD0);
}

void hi_i2c_start_tx(struct hi_i2c *pinfo, unsigned int reg_addr,
			unsigned int length)
{
	unsigned int reg;

	writel(reg_addr, pinfo->regbase + I2C_DMA_CMD1);
	writel(length, pinfo->regbase + I2C_DMA_CMD2);

	reg = readl(pinfo->regbase + I2C_DMA_CMD0);

	/*start rx*/
	writel((0xc0000000 | reg), pinfo->regbase + I2C_DMA_CMD0);
}

int dma_to_i2c(unsigned int src, unsigned int dst, unsigned int length)
{
	int chan;

	chan = do_dma_m2p(src, dst, length);
	if (chan == -1)
		hi_err("dma_to_i2c error\n");

	return chan;
}


int i2c_to_dma(unsigned int src, unsigned int dst, unsigned int length)
{
	int chan;

	chan = do_dma_p2m(dst, src, length);
	if (chan == -1)
		hi_err("dma_p2m error...\n");

	return chan;
}

/**
 * i2c_trylock_adapter - Try to get exclusive access to an I2C bus segment
 * @adapter: Target I2C bus segment
 */
static int i2c_trylock_adapter(struct i2c_adapter *adapter)
{
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);

	if (parent)
		return i2c_trylock_adapter(parent);
	else
		return rt_mutex_trylock(&adapter->bus_lock);
}

int hi_i2c_dma_write(const struct i2c_client *client, unsigned int data_addr,
		unsigned int reg_addr, unsigned int reg_addr_num,
		unsigned int length)
{
	struct i2c_adapter *adap = client->adapter;
	struct hi_i2c *pinfo = (struct hi_i2c *)i2c_get_adapdata(adap);
	unsigned int temp_reg = reg_addr;
	int chan;

	struct i2c_msg msg;

	if (in_atomic() || irqs_disabled()) {
		if (!i2c_trylock_adapter(adap))
			/* I2C activity is ongoing. */
			return -EAGAIN;
	} else {
		i2c_lock_adapter(adap);
	}

	memset(&msg, 0x0, sizeof(struct i2c_msg));
	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = length;

	pinfo->msgs = &msg;
	pinfo->msg_num = length;
	pinfo->msg_index = 0;

	/* 1. switch i2c devaddr and dma mode*/
	if (hi_i2c_set_dev_addr_and_mode(pinfo, I2C_MODE_DMA) < 0) {
		hi_err("HI_I2C_Dma_Write error...\n");
		return -1;
	}

	if (2 == reg_addr_num) {
		/* switch high byte and low byte */
		temp_reg = REVERT_HL_BYTE(reg_addr);
		writel(0x10000000, pinfo->regbase + I2C_DMA_CMD0);
	}

	/* 2. config i2c into DMA mode */
	hi_i2c_dmac_config(pinfo, 0x1);

	/* 3. start i2c logic to write */
	hi_i2c_start_tx(pinfo, temp_reg, length - 1);

	/* 4. transmit DATA from DMAC to I2C in DMA mode */
	chan = dma_to_i2c(data_addr, (pinfo->mem->start + I2C_DATA_CMD_REG),
				length);

	if (dmac_wait(chan) != DMAC_CHN_SUCCESS) {
		hi_err("dma wait failed\n");
		return -1;
	}

	dmac_channel_free(chan);

	i2c_unlock_adapter(adap);

	return 0;
}
EXPORT_SYMBOL(hi_i2c_dma_write);

int hi_i2c_dma_read(const struct i2c_client *client, unsigned int data_addr,
		unsigned int reg_addr, unsigned int reg_addr_num,
		unsigned int length)
{
	struct i2c_adapter *adap = client->adapter;
	struct hi_i2c *pinfo = (struct hi_i2c *)i2c_get_adapdata(adap);
	unsigned int temp_reg = reg_addr;
	int chan;

	struct i2c_msg msg;

	if (in_atomic() || irqs_disabled()) {
		if (!i2c_trylock_adapter(adap))
			/* I2C activity is ongoing. */
			return -EAGAIN;
	} else {
		i2c_lock_adapter(adap);
	}

	memset(&msg, 0x0, sizeof(struct i2c_msg));
	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.flags |= I2C_M_RD;
	msg.len = length;

	pinfo->msgs = &msg;
	pinfo->msg_num = length;
	pinfo->msg_index = 0;

	/* 1. switch i2c devaddr and dma mode*/
	if (hi_i2c_set_dev_addr_and_mode(pinfo, I2C_MODE_DMA) < 0)
		return -1;

	if (2 == reg_addr_num) {
		/* switch high byte and low byte */
		temp_reg = REVERT_HL_BYTE(reg_addr);
		writel(0x10000000, pinfo->regbase + I2C_DMA_CMD0);
	}

	/* 2. config i2c into DMA mode */
	hi_i2c_dmac_config(pinfo, 0x0);

	/* 3. transmit DATA from I2C to DMAC in DMA mode */
	chan = i2c_to_dma((pinfo->mem->start + I2C_DATA_CMD_REG),
				data_addr, length);

	/* 4. start i2c logic to read */
	hi_i2c_start_rx(pinfo, temp_reg, length - 1);

	if (dmac_wait(chan) != DMAC_CHN_SUCCESS)
		return -1;

	dmac_channel_free(chan);

	i2c_unlock_adapter(adap);

	return 0;
}
EXPORT_SYMBOL(hi_i2c_dma_read);
#endif

/**************************************************************/

static u32 hi_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C;
}

static const struct i2c_algorithm hi_i2c_algo = {
	.master_xfer    = hi_i2c_xfer,
	.functionality  = hi_i2c_func,
};

static int hi_i2c_probe(struct platform_device *pdev)
{
	int errorcode;
	struct hi_i2c *pinfo;
	struct i2c_adapter *adap;
	struct resource *mem;
	struct hi_platform_i2c *platform_info;

	platform_info =
		(struct hi_platform_i2c *)pdev->dev.platform_data;
	if (platform_info == NULL) {
		dev_err(&pdev->dev, "%s: Can't get platform_data!\n",
				__func__);
		errorcode = -EPERM;
		goto i2c_errorcode_na;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get I2C mem resource failed!\n");
		errorcode = -ENXIO;
		goto i2c_errorcode_na;
	}

	pinfo = kzalloc(sizeof(struct hi_i2c), GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		errorcode = -ENOMEM;
		goto i2c_errorcode_na;
	}

	pinfo->regbase = (unsigned char __iomem *)IO_ADDRESS(mem->start);
	pinfo->mem = mem;
	pinfo->dev = &pdev->dev;
	pinfo->pdata = platform_info;
	pinfo->g_last_dev_addr = 0;

	hi_i2c_hw_init(pinfo);

	platform_set_drvdata(pdev, pinfo);

	adap = &pinfo->adap;
	i2c_set_adapdata(adap, pinfo);
	adap->owner = THIS_MODULE;
	adap->class = platform_info->i2c_class;
	strlcpy(adap->name, pdev->name, sizeof(adap->name));
	adap->algo = &hi_i2c_algo;
	adap->dev.parent = &pdev->dev;
	adap->nr = pdev->id;
	adap->retries = CONFIG_HI_I2C_RETRIES;
	errorcode = i2c_add_numbered_adapter(adap);
	if (errorcode) {
		dev_err(&pdev->dev,
				"%s: Adding I2C adapter failed!\n", __func__);
		goto i2c_errorcode_free_irq;
	}

	dev_notice(&pdev->dev,
			"Hisilicon [%s] probed!\n",
			dev_name(&pinfo->adap.dev));

	goto i2c_errorcode_na;

i2c_errorcode_free_irq:
	free_irq(pinfo->irq, pinfo);
	kfree(pinfo);

i2c_errorcode_na:
	return errorcode;
}

static int hi_i2c_remove(struct platform_device *pdev)
{
	struct hi_i2c *pinfo;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		i2c_del_adapter(&pinfo->adap);

		free_irq(pinfo->irq, pinfo);

		kfree(pinfo);
	}

	dev_notice(&pdev->dev,
			"Remove Hisilicon Media Processor"
			"I2C adapter[%s] .\n",
			dev_name(&pinfo->adap.dev));

	return 0;
}

#ifdef CONFIG_PM
static int hi_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct hi_i2c *pinfo;

	pinfo = platform_get_drvdata(pdev);

	hi_i2c_abortprocess(pinfo);

	return 0;
}

static int hi_i2c_resume(struct platform_device *pdev)
{
	struct hi_i2c *pinfo;

	pinfo = platform_get_drvdata(pdev);

	hi_i2c_hw_init(pinfo);

	return 0;
}
#else
#define hi_i2c_suspend      NULL
#define hi_i2c_resume       NULL
#endif

static struct platform_driver hi_i2c_driver = {
	.probe		= hi_i2c_probe,
	.remove		= hi_i2c_remove,
	.suspend	= hi_i2c_suspend,
	.resume		= hi_i2c_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= HI_I2C,
	},
};

#ifdef CONFIG_ARCH_HI3519
#include "i2c_hi3519.c"
#endif

module_init(hi_i2c_module_init);
module_exit(hi_i2c_module_exit);

MODULE_DESCRIPTION("HISILICON I2C Bus driver");
MODULE_AUTHOR("BVT OSDRV");
MODULE_LICENSE("GPL");
