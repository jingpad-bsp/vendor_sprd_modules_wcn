/*
* Copyright (C) 2019 Spreadtrum Communications Inc.
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/param.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/termios.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include "pcie.h"
#include "pcie_test.h"


#define VERSION         "1.1"
#define PROC_DIR        "wcn/pcie"
#define TEST_BUF_SIZE	(64)

struct wcn_proc_entry {
	char *name;
	struct proc_dir_entry *entry;
};

struct wcn_proc_info {
	char *dir_name;
	struct proc_dir_entry *wcn_dir;
	struct proc_dir_entry *pcie_dir;
	struct wcn_proc_entry edma;
	struct wcn_proc_entry legacy;
	struct wcn_proc_entry msi;
	struct wcn_proc_entry throughput;
	char * test_buf;
};

/****************************** For PCIe test ********************************/
/*
 * AP read and write CP
 * bar match mode
 * use one bar
 */

/*
 * AP read and write CP
 * bar match mode
 * use two bar
 * bar0: for edma
 * bar2: for dynamic mapping
 */

/*
 * AP read and write CP
 * no use bar
 */

/*
 * CP read and write AP
 * addr match mode
 * use outbound0
 * use AXI:80 0000 0000  CPU:8000 0000
 */


/*
 * CP read and write AP
 * addr match mode
 * use outbound 0 and outbound 1
 * use AXI:80 0000 0000  CPU:8000 0000
 *
 */

/*
 * MSI irq test(2^n)
 * case 1: one msi irq
 * case 2: two msi irq
 * case 3: 16 msi irq
 * case 4. 32 msi irq
 * case 5: 64 msi irq
 */

/*
 * MSI-X irq test(2^n)
 * case 1: one msi irq
 * case 2: two msi irq
 * case 3: 16 msi irq
 * case 4. 32 msi irq
 * case 5: 64 msi irq
 */

 
/*
 * legacy irq test
 */


/**************************** For EDMA test **********************************/

/* two-link mode: edma are operated by AP and CP at the same time */


/* two-link mode: edma are operated only by AP */

/* two-link mode: edma are operated only by CP */

/* one-link mode: ap link, cp no link */


/* one-link mode: cp link, ap no link */

/* non-link mode: ap no link, cp no link */

/******************** For PCIe throughput capacity test **********************/


/****************** For PCIe+EDMA throughput capacity test *******************/


/**************************** For ASPM test *******************************/


/**************************** For PCI-PM test ******************************/

/* only for 4000 0000 ~ 4040 0000 */
static u32
sprd_pcie_read_reg32(struct wcn_pcie_info *priv, int offset)
{
	char *addr = priv->bar[0].vmem;

	addr += offset;
	/* (ioread32(address)) */
	return (readl_relaxed(addr));

}

static void
sprd_pcie_write_reg32(struct wcn_pcie_info *priv, u32 reg_offset,
			    u32 value)
{
	char *address = priv->bar[0].vmem;

	address += reg_offset;
	/* iowrite32(value, address) */
	writel_relaxed(value, address);

	return;
}

static int wcn_test_proc_open(struct inode *inode, struct file *filp)
{
	struct wcn_proc_entry *entry =
		(struct wcn_proc_entry *)PDE_DATA(inode);
	filp->private_data = entry;

	pr_info("%s: name = %s,p=%p\n", __func__, entry->name, entry);

	return 0;
}

static int wcn_test_proc_release(struct inode *inode, struct file *filp)
{
	pr_info("%s\n", __func__);

	return 0;
}

static ssize_t wcn_test_proc_read(struct file *filp,
		char __user *buf, size_t count, loff_t *ppos)
{
	pr_info("%s: count=%ld\n", __func__, count);

	return count;
}

int edma_test_init(void)
{
	struct wcn_pcie_info *pdev;
	u32 value;

	pdev = get_wcn_device_info();
	if (!pdev) {
		pr_err("%s:pcie device is null\n", __func__);
		return -1;
	}
	value = sprd_pcie_read_reg32(pdev, AHB_EB0);
	pr_info("AHB_EB0=0x%x\n", value);
	value = value | EDMA_EB | PCIE_EB;
	sprd_pcie_write_reg32(pdev, AHB_EB0, value);

	return 0;
}

int edma_test_deinit(void)
{
	struct wcn_pcie_info *pdev;
	u32 value;

	pdev = get_wcn_device_info();
	if (!pdev) {
		pr_err("%s:pcie device is null\n", __func__);
		return -1;
	}
	value = sprd_pcie_read_reg32(pdev, AHB_EB0);
	value = value & ~EDMA_EB ;
	sprd_pcie_write_reg32(pdev, AHB_EB0, value);

	return 0;
}


int edma_chn_test_init(int chn)
{
	struct wcn_pcie_info *pdev;
	u32 value;

	pdev = get_wcn_device_info();
	if (!pdev) {
		pr_err("%s:pcie device is null\n", __func__);
		return -1;
	}
	value = sprd_pcie_read_reg32(pdev, CHN_DMA_CFG(chn));
	value = value & ~EDMA_EB ;
	sprd_pcie_write_reg32(pdev, AHB_EB0, value);
#if 0
	int ret, dir = 0;
	struct dscr_ring *dscr_ring;
	struct edma_info *edma = edma_info();
	union dma_chn_int_reg dma_int = { 0 };
	union dma_chn_cfg_reg dma_cfg = { 0 };
	struct desc local_DSCR;

	if (inout == RX)
		/* int direction. int send to ap */
		dir = 1;
	WCN_INFO("[+]%s(chn=%d,mode=%d,dir=%d,inout=%d,max_trans=%d)\n",
		 __func__, chn, mode, dir, inout, max_trans);

	dma_int.reg = edma->dma_chn_reg[chn].dma_int.reg;
	dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
	local_DSCR = edma->dma_chn_reg[chn].dma_dscr;

	switch (mode) {
	case TWO_LINK_MODE:
		dscr_ring = &(edma->chn_sw[chn].dscr_ring);
		ret = dscr_ring_init(dscr_ring, inout, max_trans, NULL);
		if (ret)
			return ERROR;
		/* 1:enable channel; 0:disable channel */
		if (dma_cfg.bit.rf_chn_en == 0) {
			dma_cfg.bit.rf_chn_en = 1;
			/* 0:trans done, 1:linklist done */
			dma_cfg.bit.rf_chn_req_mode = 1;
			dma_cfg.bit.rf_chn_list_mode = TWO_LINK_MODE;
			if (!inout)
			    /* source data from CP */
				dma_cfg.bit.rf_chn_dir = 1;
			else
				/* source data from AP */
				dma_cfg.bit.rf_chn_dir = 0;
		}
		if (inout == TX) {
			/* tx_list link point */
			local_DSCR.rf_chn_tx_next_dscr_ptr_low =
			    GET_32_OF_40((unsigned char *)(dscr_ring->head));
			/* tx_list link point */
			local_DSCR.chn_ptr_high.bit
						.rf_chn_tx_next_dscr_ptr_high =
					GET_8_OF_40(dscr_ring->head);
			dma_int.bit.rf_chn_tx_complete_int_en = 1;
			dma_int.bit.rf_chn_tx_pop_int_clr = 1;
			dma_int.bit.rf_chn_tx_complete_int_clr = 1;
		} else {
			local_DSCR.rf_chn_rx_next_dscr_ptr_low =
			    GET_32_OF_40((unsigned char *)(dscr_ring->head));
			local_DSCR.chn_ptr_high.bit
						.rf_chn_rx_next_dscr_ptr_high =
					GET_8_OF_40(dscr_ring->head);
			dma_int.bit.rf_chn_rx_push_int_en = 1;
			/* clear semaphore value */
			dma_cfg.bit.rf_chn_sem_value = 0xFF;
		}
		edma_pending_q_init(chn, mchn_hw_max_pending(chn));
		break;
	case ONE_LINK_MODE:
		/* 0:to cp . 1:to AP */
		dma_cfg.bit.rf_chn_int_out_sel = dir;
		/* 1:enable channel; 0:disable channel */
		dma_cfg.bit.rf_chn_en = 1;
		/* 0:trans done, 1:linklist done */
		dma_cfg.bit.rf_chn_req_mode = 1;
		dma_cfg.bit.rf_chn_list_mode = ONE_LINK_MODE;
		dma_int.bit.rf_chn_tx_complete_int_en = 1;
		dma_int.bit.rf_chn_tx_pop_int_en = 1;
		break;

	case NON_LINK_MODE:
	case 3:
		dma_int.bit.rf_chn_tx_complete_int_en = 0;
		dma_int.bit.rf_chn_tx_pop_int_en = 0;
		dma_int.bit.rf_chn_rx_push_int_en = 0;
		dma_int.bit.rf_chn_rx_pop_int_en = 0;
		/* 0:tx_int to CP; 1:rx_int to AP */
		dma_cfg.bit.rf_chn_int_out_sel = dir;
		dma_cfg.bit.rf_chn_en = 1;
		dma_cfg.bit.rf_chn_list_mode = NON_LINK_MODE;
		break;
	default:
		break;
	}

	switch (mode) {
	case TWO_LINK_MODE:
		if (inout) {
			edma->dma_chn_reg[chn].dma_dscr
					      .rf_chn_tx_next_dscr_ptr_low =
					local_DSCR.rf_chn_tx_next_dscr_ptr_low;
			edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.bit
						.rf_chn_tx_next_dscr_ptr_high =
									0x80;
		} else {
			edma->dma_chn_reg[chn].dma_dscr
					      .rf_chn_rx_next_dscr_ptr_low =
					local_DSCR.rf_chn_rx_next_dscr_ptr_low;
			edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.bit
						.rf_chn_rx_next_dscr_ptr_high =
									0x80;
		}
		edma->dma_chn_reg[chn].dma_dscr.chn_ptr_high.reg =
		    local_DSCR.chn_ptr_high.reg;

		break;
	default:
		break;
	}
	edma->chn_sw[chn].dir = dir;
	edma->chn_sw[chn].inout = inout;
	edma->chn_sw[chn].mode = mode;
	edma->dma_chn_reg[chn].dma_int.reg = dma_int.reg;
	edma->dma_chn_reg[chn].dma_cfg.reg = dma_cfg.reg;
	dma_cfg.reg = edma->dma_chn_reg[chn].dma_cfg.reg;
	WCN_INFO("[-]%s\n", __func__);
#endif
	return 0;
}


int edma_chn_test_deinit(int chn)
{
	return 0;

}


static ssize_t handle_edma_test(struct file *filp,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct wcn_proc_entry *entry = (struct wcn_proc_entry *)filp->private_data;
	struct wcn_proc_info *p_proc;


	p_proc = container_of(entry, struct wcn_proc_info, edma);
	memset(p_proc->test_buf, 0, TEST_BUF_SIZE);
	if (copy_from_user(p_proc->test_buf, buf, count))
		return -EFAULT;

	if (strncmp(p_proc->test_buf, "edma_2_ap_cp", 12) == 0) {
		pr_info("edma_2_ap_cp comming\n");
	}

	pr_info("test_buf:%s\n", p_proc->test_buf);

	return 0;
}
static ssize_t wcn_test_proc_write(struct file *filp,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct wcn_proc_entry *entry = (struct wcn_proc_entry *)filp->private_data;
	char *type = entry->name;
	int pos = *ppos;

	pr_info("%s: count =%ld, name=%p, name =%s, pos=%d\n", __func__, count, entry, type, pos);

	if (count > TEST_BUF_SIZE)
		return -EFAULT;

	if (strcmp(type, "edma") == 0) {
		pr_info("%s: edma test begain\n", __func__);
		handle_edma_test(filp, buf, count, ppos);
	}

	return count;
}

static unsigned int wcn_test_proc_poll(struct file *filp, poll_table *wait)
{
	pr_info("%s\n", __func__);

	return 0;
}
static long wcn_test_proc_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	pr_info("%s\n", __func__);
	return 0;
}

static const struct file_operations wcn_test_proc_fops = {
	.open		= wcn_test_proc_open,
	.release		= wcn_test_proc_release,
	.read		= wcn_test_proc_read,
	.write		= wcn_test_proc_write,
	.poll		= wcn_test_proc_poll,
	.unlocked_ioctl	= wcn_test_proc_ioctl,
	.compat_ioctl	= wcn_test_proc_ioctl,
};
/*
[  131.364988] wcn_test_proc_open: name = legacy,p=ffffffc0f76edc28
[  131.365029] wcn_test_proc_write: count =2, name=ffffffc0f76edc28, name =legacy
[  131.365038] wcn_test_proc_release

[  172.838564] wcn_test_proc_open: name = edma,p=ffffffc0f76edc18
[  172.838604] wcn_test_proc_write: count =2, name=ffffffc0f76edc18, name =edma
[  172.838613] wcn_test_proc_release
*/
/*
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init wcn_pcie_test_init(void)
{
	int ret;
	struct wcn_proc_info *p_proc;

	p_proc = kzalloc(sizeof(struct wcn_proc_info), GFP_KERNEL);
	if (!p_proc)
		return -ENOMEM;
	p_proc->test_buf = kzalloc(TEST_BUF_SIZE, GFP_KERNEL);
	if (!p_proc->test_buf )
		return -ENOMEM;

	p_proc->wcn_dir = proc_mkdir("wcn", NULL);
	if (!p_proc->wcn_dir) {
		pr_err("Unable to create /proc/wcn directory");
		return -ENOMEM;
	}

	p_proc->pcie_dir = proc_mkdir("pcie", p_proc->wcn_dir);
	if (!p_proc->pcie_dir) {
		pr_err("Unable to create /proc/%s directory", PROC_DIR);
		return -ENOMEM;
	}

	/* Creating read/write  entry */
	p_proc->edma.name = "edma";
	p_proc->edma.entry = proc_create_data("edma", S_IRUGO | S_IWUSR | S_IWGRP,
		p_proc->pcie_dir, &wcn_test_proc_fops, &p_proc->edma);

	p_proc->legacy.name = "legacy";
	p_proc->legacy.entry = proc_create_data("legacy", S_IRUGO | S_IWUSR | S_IWGRP,
		p_proc->pcie_dir, &wcn_test_proc_fops, &p_proc->legacy);
#if 0
	return ret;

fail:
	remove_proc_entry("throughput", wcn_dir);
	remove_proc_entry("legacy", wcn_dir);
	remove_proc_entry("edma", wcn_dir);

	remove_proc_entry("wcn", 0);
#endif
	return ret;
}

/* Cleans up the module. */
static void __exit wcn_pcie_test_exit(void)
{
	
}

module_init(wcn_pcie_test_init);
module_exit(wcn_pcie_test_exit);
MODULE_DESCRIPTION("WCN Pcie Test Driver ver %s " VERSION);
MODULE_LICENSE("GPL");

