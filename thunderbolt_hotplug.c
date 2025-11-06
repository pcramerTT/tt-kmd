// thunderbolt_hotplug.c
#include "thunderbolt.h"
#include "enumerate.h"
#include <linux/pci.h>

void tenstorrent_tb_hotplug_work(struct work_struct *work)
{
    struct tenstorrent_tb_device *tt_tb_dev = 
        container_of(work, struct tenstorrent_tb_device, hotplug_work);
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    int ret;

    // For Thunderbolt AI accelerator cards, we primarily need to handle
    // device connection/disconnection events and ensure proper initialization
    
    if (tt_dev->detached) {
        // Device was disconnected, mark as reconnected
        dev_info(&tt_dev->dev, "Tenstorrent AI accelerator reconnected\n");
        tt_dev->detached = false;
        
        // Reinitialize the AI accelerator hardware
        if (tt_dev->needs_hw_init && tt_dev->dev_class->init_hardware) {
            ret = tt_dev->dev_class->init_hardware(tt_dev);
            if (ret) {
                dev_err(&tt_dev->dev, "Failed to reinitialize AI accelerator\n");
                tt_dev->needs_hw_init = true;
            } else {
                tt_dev->needs_hw_init = false;
                dev_info(&tt_dev->dev, "AI accelerator ready for compute\n");
            }
        }
    } else {
        // Check if device is still responsive
        dev_info(&tt_dev->dev, "AI accelerator hotplug event\n");
        
        // For AI accelerators, we might need to reset or reinitialize
        // the compute context when hotplug events occur
        if (tt_dev->dev_class->tb_handle_hotplug) {
            tt_dev->dev_class->tb_handle_hotplug(tt_dev, true);
        }
    }
}

// For Thunderbolt AI accelerators, hotplug is handled through the probe/remove
// callbacks in the Thunderbolt driver framework. The work queue can be triggered
// manually when needed for device state management.

// Function to manually trigger hotplug work (e.g., from probe/remove handlers)
void tenstorrent_tb_trigger_hotplug_work(struct tenstorrent_tb_device *tt_tb_dev)
{
    if (tt_tb_dev->hotplug_supported) {
        schedule_work(&tt_tb_dev->hotplug_work);
    }
}

// Function to mark device as disconnected (called from remove handler)
void tenstorrent_tb_mark_disconnected(struct tenstorrent_tb_device *tt_tb_dev)
{
    struct tenstorrent_device *tt_dev = &tt_tb_dev->tt_dev;
    
    if (tt_tb_dev->hotplug_supported) {
        dev_info(&tt_dev->dev, "Tenstorrent AI accelerator disconnected\n");
        tt_dev->detached = true;
        // Cancel any pending work since device is disconnected
        cancel_work_sync(&tt_tb_dev->hotplug_work);
    }
}
