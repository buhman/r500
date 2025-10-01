#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/aperture.h>

#define R500 "r500"

static struct pci_device_id r500_id_table[] = {
  { PCI_DEVICE(0x1002, 0x71c1) },
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

static inline void wreg(void __iomem * rmmio, uint32_t reg, uint32_t v)
{
  writel(v, ((void __iomem *)rmmio) + reg);
}

static inline uint32_t rreg(void __iomem * rmmio, uint32_t reg)
{
  return readl(((void __iomem *)rmmio) + reg);
}

/* This function is called by the kernel */
static int r500_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
  u16 vendor, device;
  pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word(pdev, PCI_DEVICE_ID, &device);
  printk(KERN_INFO "[r500] VENDOR_ID: %04x DEVICE_ID: %04x\n", vendor, device);

  struct resource mem;
  int ret;

  ret = aperture_remove_conflicting_pci_devices(pdev, "R500");
  if (ret)
    return ret;

  ret = pci_enable_device(pdev);
  if (ret)
    goto err_free;

  resource_size_t rmmio_base = pci_resource_start(pdev, 2);
  resource_size_t rmmio_size = pci_resource_len(pdev, 2);

  void __iomem * rmmio = ioremap(rmmio_base, rmmio_size);
  printk(KERN_INFO "[r500] rmmio base: %08x ; rmmio size: %08x\n", rmmio_base, rmmio_size);

  uint32_t value1 = rreg(rmmio, 0x6080);
  printk(KERN_INFO "[r500] D1CRTC_CONTROL %08x\n", value1);
  uint32_t value2 = rreg(rmmio, 0x6880);
  printk(KERN_INFO "[r500] D2CRTC_CONTROL %08x\n", value2);

  wreg(rmmio, 0x6080, 1);

 err_free:
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
