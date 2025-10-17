// thunderbolt_pcie.c
#include "thunderbolt.h"

static int tenstorrent_tb_setup_pcie_tunneling(struct tenstorrent_tb_device *tt_tb_dev)
{
    struct tb_xdomain *xdomain = tt_tb_dev->xdomain;
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    int ret;

    // Set up PCIe tunneling through Thunderbolt
    ret = tb_xdomain_enable_pcie_tunneling(xdomain);
    if (ret) {
        dev_err(&tt_tb_dev->tb_dev->dev, "Failed to enable PCIe tunneling\n");
        return ret;
    }

    // Map PCIe configuration space
    ret = tenstorrent_tb_map_pcie_config(tt_tb_dev);
    if (ret) {
        tb_xdomain_disable_pcie_tunneling(xdomain);
        return ret;
    }

    // Set up BAR mappings through Thunderbolt
    ret = tenstorrent_tb_map_bars(tt_tb_dev);
    if (ret) {
        tenstorrent_tb_unmap_pcie_config(tt_tb_dev);
        tb_xdomain_disable_pcie_tunneling(xdomain);
        return ret;
    }

    return 0;
}

static int tenstorrent_tb_map_pcie_config(struct tenstorrent_tb_device *tt_tb_dev)
{
    struct tb_xdomain *xdomain = tt_tb_dev->xdomain;
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    u16 vendor_id, device_id;
    int ret;

    // Read PCIe configuration space through Thunderbolt
    ret = tb_xdomain_pcie_read(xdomain, PCI_VENDOR_ID, &vendor_id, 2);
    if (ret) {
        dev_err(&tt_tb_dev->tb_dev->dev, "Failed to read vendor ID\n");
        return ret;
    }

    ret = tb_xdomain_pcie_read(xdomain, PCI_DEVICE_ID, &device_id, 2);
    if (ret) {
        dev_err(&tt_tb_dev->tb_dev->dev, "Failed to read device ID\n");
        return ret;
    }

    if (vendor_id != PCI_VENDOR_ID_TENSTORRENT) {
        dev_err(&tt_tb_dev->tb_dev->dev, "Invalid vendor ID: 0x%04x\n", vendor_id);
        return -ENODEV;
    }

    dev_info(&tt_tb_dev->tb_dev->dev, "Found Tenstorrent device 0x%04x via Thunderbolt\n", device_id);

    return 0;
}

static int tenstorrent_tb_map_bars(struct tenstorrent_tb_device *tt_tb_dev)
{
    struct tb_xdomain *xdomain = tt_tb_dev->xdomain;
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    int i, ret;

    // Map PCIe BARs through Thunderbolt tunneling
    for (i = 0; i < PCI_NUM_RESOURCES; i++) {
        u32 bar_val;
        resource_size_t bar_start, bar_len;

        ret = tb_xdomain_pcie_read(xdomain, PCI_BASE_ADDRESS_0 + i * 4, &bar_val, 4);
        if (ret)
            continue;

        if (bar_val == 0)
            continue;

        // Determine BAR size and type
        tb_xdomain_pcie_write(xdomain, PCI_BASE_ADDRESS_0 + i * 4, 0xFFFFFFFF, 4);
        tb_xdomain_pcie_read(xdomain, PCI_BASE_ADDRESS_0 + i * 4, &bar_val, 4);
        tb_xdomain_pcie_write(xdomain, PCI_BASE_ADDRESS_0 + i * 4, bar_val, 4);

        bar_len = ~(bar_val & 0xFFFFFFF0) + 1;
        bar_start = bar_val & 0xFFFFFFF0;

        // Map BAR through Thunderbolt
        ret = tb_xdomain_map_bar(xdomain, i, bar_start, bar_len);
        if (ret) {
            dev_err(&tt_tb_dev->tb_dev->dev, "Failed to map BAR %d\n", i);
            return ret;
        }
    }

    return 0;
}
