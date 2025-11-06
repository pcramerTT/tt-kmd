// thunderbolt_memory.c
#include "thunderbolt.h"
#include "chardev_private.h"
#include "memory.h"

// For Thunderbolt AI accelerators, memory mapping is handled through the
// standard memory mapping functions. The Thunderbolt tunneling is set up
// during device initialization in thunderbolt_pcie.c, which makes the
// device appear as a standard PCI device for memory mapping purposes.

// Thunderbolt-specific mmap function
// For AI accelerators over Thunderbolt, we delegate to the standard mmap
// since the BARs are already set up through Thunderbolt tunneling.
int tenstorrent_tb_mmap(struct chardev_private *priv, struct vm_area_struct *vma)
{
    struct tenstorrent_device *tt_dev = priv->device;
    
    // For Thunderbolt devices, we need to ensure the device isn't detached
    if (tt_dev->detached) {
        dev_err(&tt_dev->dev, "Cannot map memory: device is detached\n");
        return -ENODEV;
    }
    
    // For Thunderbolt devices, pdev should be set during device initialization
    // If it's not set, the device isn't properly initialized
    if (!tt_dev->pdev) {
        dev_err(&tt_dev->dev, "Cannot map memory: PCI device not initialized\n");
        return -ENODEV;
    }
    
    // Delegate to the standard memory mapping function
    // The Thunderbolt tunneling setup makes the device appear as a standard
    // PCI device for memory mapping purposes
    return tenstorrent_mmap(priv, vma);
}
