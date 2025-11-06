// thunderbolt_pcie.c
#include "thunderbolt.h"
#include "enumerate.h"

// For Thunderbolt AI accelerators, PCIe tunneling is handled automatically
// by the Thunderbolt subsystem when the device is probed. This file provides
// a simplified interface for any Thunderbolt-specific PCIe setup needed.

// Setup PCIe tunneling for Thunderbolt AI accelerator
// Note: The Thunderbolt driver framework handles most of the PCIe tunneling
// setup automatically. This function can be used for any additional setup needed.
int tenstorrent_tb_setup_pcie_tunneling(struct tenstorrent_tb_device *tt_tb_dev)
{
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    
    // The Thunderbolt subsystem automatically sets up PCIe tunneling
    // when the device is probed. We just need to verify the device is ready.
    if (tt_dev->detached) {
        dev_err(&tt_dev->dev, "Cannot setup PCIe tunneling: device is detached\n");
        return -ENODEV;
    }
    
    dev_info(&tt_dev->dev, "PCIe tunneling ready for Thunderbolt AI accelerator\n");
    return 0;
}

// Cleanup PCIe tunneling (called from remove handler)
void tenstorrent_tb_cleanup_pcie_tunneling(struct tenstorrent_tb_device *tt_tb_dev)
{
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    
    // The Thunderbolt subsystem handles cleanup automatically
    // This function is provided for symmetry and any additional cleanup needed
    dev_info(&tt_dev->dev, "PCIe tunneling cleanup for Thunderbolt AI accelerator\n");
}
