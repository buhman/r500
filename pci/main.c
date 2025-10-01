#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>

#define R500 "r500"

static struct pci_device_id r500_id_table[] = {
  { PCI_DEVICE(0x121a, 0x0002) },
  { 0,}
};

MODULE_DEVICE_TABLE(pci, r500_id_table);

static int r500_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void r500_remove(struct pci_dev *pdev);

static struct pci_driver r500 = {
  .name = R500,
  .id_table = r500_id_table,
  .probe = r500_probe,
  .remove = r500_remove
};

struct r500_priv {
  volatile u32 __iomem *hwmem;
};

/* */

static int __init r500_module_init(void)
{
  return pci_register_driver(&r500);
}

static void __exit r500_module_exit(void)
{
  pci_unregister_driver(&r500);
}

void release_device(struct pci_dev *pdev);

void release_device(struct pci_dev *pdev)
{
  /* Free memory region */
  pci_release_region(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
  /* And disable device */
  pci_disable_device(pdev);
}

/* This function is called by the kernel */
static int r500_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
  int bar, err;
  u16 vendor, device;
  unsigned long mmio_start, mmio_len;

  struct r500_priv *drv_priv;

  pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word(pdev, PCI_DEVICE_ID, &device);

  printk(KERN_INFO "vid: %04x pid: %04x\n", vendor, device);

  /* Request IO BAR */
  bar = pci_select_bars(pdev, IORESOURCE_MEM);

  /* Enable device memory */
  err = pci_enable_device_mem(pdev);
  if (err) {
    printk(KERN_INFO "pci_enable_device_mem error\n");
    return err;
  }

  /* Request memory region for the BAR */
  err = pci_request_region(pdev, bar, R500);
  if (err) {
    printk(KERN_INFO "pci_request_region error\n");
    pci_disable_device(pdev);
    return err;
  }

  /* Get start and stop memory offsets */
  mmio_start = pci_resource_start(pdev, 0);
  mmio_len = pci_resource_len(pdev, 0);
  printk(KERN_INFO "mmio_start %p mmio_len %p\n", (void*)mmio_start, (void*)mmio_len);

  /* Allocate memory for the module private data */
  drv_priv = kzalloc(sizeof(struct r500_priv), GFP_KERNEL);
  if (!drv_priv) {
    release_device(pdev);
    return -ENOMEM;
  }

  /* Remap BAR to the local pointer */
  drv_priv->hwmem = ioremap(mmio_start, mmio_len);
  if (!drv_priv->hwmem) {
    release_device(pdev);
    return -EIO;
  }

  /* Set module private data */
  /* Now we can access mapped "hwmem" from the any module's function */
  pci_set_drvdata(pdev, drv_priv);

  return 0;
}

/* Clean up */
static void r500_remove(struct pci_dev *pdev)
{
  struct r500_priv *drv_priv = pci_get_drvdata(pdev);

  if (drv_priv) {
    if (drv_priv->hwmem) {
      iounmap(drv_priv->hwmem);
    }

    kfree(drv_priv);
  }

  release_device(pdev);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zachary Buhman <zack@buhman.org>");
MODULE_DESCRIPTION("R500 module");
MODULE_VERSION("0.1");

module_init(r500_module_init);
module_exit(r500_module_exit);
