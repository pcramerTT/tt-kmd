// thunderbolt.h
#ifndef TTDRIVER_THUNDERBOLT_H_INCLUDED
#define TTDRIVER_THUNDERBOLT_H_INCLUDED

#include <linux/thunderbolt.h>
#include "device.h"

#define TB_VENDOR_ID_TENSTORRENT 0x1E52
#define TB_DEVICE_ID_WORMHOLE    0x401E
#define TB_DEVICE_ID_BLACKHOLE   0xB140

struct tenstorrent_tb_device {
    struct tenstorrent_device tt_dev;
    struct tb_device *tb_dev;
    struct tb_xdomain *xdomain;
    bool hotplug_supported;
    struct work_struct hotplug_work;
};

int tenstorrent_tb_register_driver(void);
void tenstorrent_tb_unregister_driver(void);

#endif
