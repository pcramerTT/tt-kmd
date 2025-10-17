// thunderbolt_hotplug.c
#include "thunderbolt.h"

static void tenstorrent_tb_hotplug_work(struct work_struct *work)
{
    struct tenstorrent_tb_device *tt_tb_dev = 
        container_of(work, struct tenstorrent_tb_device, hotplug_work);
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    struct tb_xdomain *xdomain = tt_tb_dev->xdomain;
    u16 vendor_id;
    int ret;

    // Check if device is still present
    ret = tb_xdomain_pcie_read(xdomain, PCI_VENDOR_ID, &vendor_id, 2);
    if (ret || vendor_id != PCI_VENDOR_ID_TENSTORRENT) {
        dev_info(&tt_tb_dev->tb_dev->dev, "Device disconnected\n");
        tt_dev->detached = true;
        return;
    }

    // Handle device reconnection
    if (tt_dev->detached) {
        dev_info(&tt_tb_dev->tb_dev->dev, "Device reconnected\n");
        tt_dev->detached = false;
        
        // Reinitialize hardware if needed
        if (tt_dev->needs_hw_init) {
            tt_dev->dev_class->init_hardware(tt_dev);
            tt_dev->needs_hw_init = false;
        }
    }
}

static int tenstorrent_tb_hotplug_notify(struct notifier_block *nb,
                                        unsigned long action, void *data)
{
    struct tenstorrent_tb_device *tt_tb_dev = 
        container_of(nb, struct tenstorrent_tb_device, hotplug_notifier);

    switch (action) {
    case TB_NOTIFY_HOTPLUG_ADD:
    case TB_NOTIFY_HOTPLUG_REMOVE:
        schedule_work(&tt_tb_dev->hotplug_work);
        break;
    }

    return NOTIFY_OK;
}
