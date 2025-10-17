// thunderbolt.c
#include "thunderbolt.h"
#include "enumerate.h"
#include "module.h"

static const struct tb_device_id tenstorrent_tb_ids[] = {
    { TB_DEVICE(TB_VENDOR_ID_TENSTORRENT, TB_DEVICE_ID_WORMHOLE),
      .driver_data = (kernel_ulong_t)&wormhole_class },
    { TB_DEVICE(TB_VENDOR_ID_TENSTORRENT, TB_DEVICE_ID_BLACKHOLE),
      .driver_data = (kernel_ulong_t)&blackhole_class },
    { 0 },
};

static int tenstorrent_tb_probe(struct tb_device *tb_dev)
{
    struct tenstorrent_tb_device *tt_tb_dev;
    struct tenstorrent_device *tt_dev;
    const struct tenstorrent_device_class *device_class;
    struct tb_xdomain *xdomain;
    int ret;

    // Get the xdomain for PCIe tunneling
    xdomain = tb_xdomain_get(tb_dev);
    if (!xdomain) {
        dev_err(&tb_dev->dev, "No xdomain found\n");
        return -ENODEV;
    }

    device_class = (const struct tenstorrent_device_class *)tb_dev->driver_data;
    if (!device_class) {
        dev_err(&tb_dev->dev, "Unsupported device\n");
        tb_xdomain_put(xdomain);
        return -ENODEV;
    }

    tt_tb_dev = kzalloc(sizeof(*tt_tb_dev), GFP_KERNEL);
    if (!tt_tb_dev) {
        tb_xdomain_put(xdomain);
        return -ENOMEM;
    }

    tt_dev = &tt_tb_dev->tt_dev;
    tt_tb_dev->tb_dev = tb_dev;
    tt_tb_dev->xdomain = xdomain;

    // Initialize the base tenstorrent device
    kref_init(&tt_dev->kref);
    tt_dev->detached = false;
    tt_dev->dev_class = device_class;
    tt_dev->ordinal = idr_alloc(&tenstorrent_dev_idr, tt_dev, 0, 0, GFP_KERNEL);
    
    if (tt_dev->ordinal < 0) {
        ret = tt_dev->ordinal;
        goto err_free;
    }

    mutex_init(&tt_dev->chardev_mutex);
    mutex_init(&tt_dev->iatu_mutex);

    // Thunderbolt-specific initialization
    tt_tb_dev->hotplug_supported = true;
    INIT_WORK(&tt_tb_dev->hotplug_work, tenstorrent_tb_hotplug_work);

    // Set up PCIe tunneling through Thunderbolt
    ret = tenstorrent_tb_setup_pcie_tunneling(tt_tb_dev);
    if (ret)
        goto err_cleanup;

    // Initialize device hardware
    if (device_class->init_device(tt_dev))
        tt_dev->needs_hw_init = !device_class->init_hardware(tt_dev);

    tenstorrent_register_device(tt_dev);

    return 0;

err_cleanup:
    idr_remove(&tenstorrent_dev_idr, tt_dev->ordinal);
err_free:
    tb_xdomain_put(xdomain);
    kfree(tt_tb_dev);
    return ret;
}

static void tenstorrent_tb_remove(struct tb_device *tb_dev)
{
    struct tenstorrent_tb_device *tt_tb_dev = tb_get_drvdata(tb_dev);
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;

    // Cancel hotplug work
    cancel_work_sync(&tt_tb_dev->hotplug_work);

    // Clean up PCIe tunneling
    tenstorrent_tb_cleanup_pcie_tunneling(tt_tb_dev);

    // Clean up device
    tt_dev->dev_class->cleanup_hardware(tt_dev);
    tt_dev->dev_class->cleanup_device(tt_dev);

    tenstorrent_unregister_device(tt_dev);
    tenstorrent_disable_interrupts(tt_dev);

    idr_remove(&tenstorrent_dev_idr, tt_dev->ordinal);
    tb_xdomain_put(tt_tb_dev->xdomain);
    kfree(tt_tb_dev);
}

static struct tb_driver tenstorrent_tb_driver = {
    .name = "tenstorrent-tb",
    .id_table = tenstorrent_tb_ids,
    .probe = tenstorrent_tb_probe,
    .remove = tenstorrent_tb_remove,
};

int tenstorrent_tb_register_driver(void)
{
    return tb_register_driver(&tenstorrent_tb_driver);
}

void tenstorrent_tb_unregister_driver(void)
{
    tb_unregister_driver(&tenstorrent_tb_driver);
}
