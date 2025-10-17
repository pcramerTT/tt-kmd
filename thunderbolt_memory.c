// thunderbolt_memory.c
#include "thunderbolt.h"

static int tenstorrent_tb_mmap(struct chardev_private *priv, struct vm_area_struct *vma)
{
    struct tenstorrent_tb_device *tt_tb_dev = 
        container_of(priv->device, struct tenstorrent_tb_device, tt_dev);
    struct tb_xdomain *xdomain = tt_tb_dev->xdomain;
    resource_size_t offset = (u64)vma->vm_pgoff << PAGE_SHIFT;
    resource_size_t size = vma->vm_end - vma->vm_start;
    int ret;

    // Map through Thunderbolt tunneling instead of direct PCIe
    ret = tb_xdomain_map_memory(xdomain, offset, size, vma);
    if (ret) {
        dev_err(&priv->device->dev, "Failed to map memory through Thunderbolt\n");
        return ret;
    }

    vma->vm_ops = &tenstorrent_tb_vm_ops;
    vma->vm_private_data = tt_tb_dev;

    return 0;
}

static const struct vm_operations_struct tenstorrent_tb_vm_ops = {
    .open = tenstorrent_tb_vma_open,
    .close = tenstorrent_tb_vma_close,
    .fault = tenstorrent_tb_vma_fault,
};

static int tenstorrent_tb_vma_fault(struct vm_fault *vmf)
{
    struct tenstorrent_tb_device *tt_tb_dev = vmf->vma->vm_private_data;
    struct tb_xdomain *xdomain = tt_tb_dev->xdomain;
    pfn_t pfn;
    int ret;

    // Handle page faults through Thunderbolt
    ret = tb_xdomain_handle_page_fault(xdomain, vmf->address, &pfn);
    if (ret)
        return VM_FAULT_SIGBUS;

    vmf->page = pfn_to_page(pfn);
    return 0;
}
