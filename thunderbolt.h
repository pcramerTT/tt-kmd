// thunderbolt.h
#ifndef TTDRIVER_THUNDERBOLT_H_INCLUDED
#define TTDRIVER_THUNDERBOLT_H_INCLUDED

#include <linux/thunderbolt.h>
#include "device.h"

struct chardev_private;
struct vm_area_struct;

#define TB_VENDOR_ID_TENSTORRENT 0x1E52
#define TB_DEVICE_ID_WORMHOLE    0x401E
#define TB_DEVICE_ID_BLACKHOLE   0xB140

struct tenstorrent_tb_device {
    struct tenstorrent_device tt_dev;
    struct tb_device *tb_dev;
    struct tb_xdomain *xdomain;
    bool hotplug_supported;
    struct work_struct hotplug_work;
    struct notifier_block hotplug_notifier;
};

int tenstorrent_tb_register_driver(void);
void tenstorrent_tb_unregister_driver(void);

// PCIe tunneling functions for Thunderbolt AI accelerators
int tenstorrent_tb_setup_pcie_tunneling(struct tenstorrent_tb_device *tt_tb_dev);
void tenstorrent_tb_cleanup_pcie_tunneling(struct tenstorrent_tb_device *tt_tb_dev);

// Hotplug functions for AI accelerator cards
void tenstorrent_tb_hotplug_work(struct work_struct *work);
void tenstorrent_tb_trigger_hotplug_work(struct tenstorrent_tb_device *tt_tb_dev);
void tenstorrent_tb_mark_disconnected(struct tenstorrent_tb_device *tt_tb_dev);

// Memory mapping for Thunderbolt AI accelerators
int tenstorrent_tb_mmap(struct chardev_private *priv, struct vm_area_struct *vma);

#endif
