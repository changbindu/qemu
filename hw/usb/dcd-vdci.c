#include "qemu/osdep.h"
#include "hw/hw.h"
#include "qemu/timer.h"
#include "hw/usb.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"
#include "trace.h"

#define VDCI_BAR0_SIZE 0x100000
#define VDCI_BAR1_SIZE 0x100000

#define TYPE_VDCI "usb-vdci"

#define VDCI(obj) \
    OBJECT_CHECK(VDCIState, (obj), TYPE_VDCI)

typedef struct {
	PCIDevice parent_obj;
	MemoryRegion bar0;
	MemoryRegion bar1;
} VDCIState;

static uint64_t usb_vdci_io_bar0_read(void *opaque, hwaddr addr, unsigned
		   size)
{
	VDCIState *s = opaque;
	PCIDevice *pci_dev = PCI_DEVICE(s);
	uint64_t ret = 0;

	printf("Read BAR0 [%lx], size %d\n",(uint64_t) addr, size);
	pci_irq_deassert(pci_dev);

	return ret; /* Returns value of the read register */
}

static void usb_vdci_io_bar0_write(void *opaque, hwaddr addr,
		    uint64_t val, unsigned size)
{
	VDCIState *s = opaque;
	PCIDevice *pci_dev = PCI_DEVICE(s);

	printf("BAR0 write [%lx] = %lx, size %d\n",
	       (uint64_t) addr, val, size);
	pci_irq_assert(pci_dev);
}

static const MemoryRegionOps b0_ops = {
	.read = usb_vdci_io_bar0_read,
	.write = usb_vdci_io_bar0_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.impl = {
		.min_access_size = 4,
		.max_access_size = 4,
	},
};

static void usb_vdci_pci_realize(PCIDevice *pci_dev, Error **errp)
{
	VDCIState *s = VDCI(pci_dev);

	printf("Starting %s realize...\n", TYPE_VDCI);

	/* Interrupt pin A */
	pci_dev->config[PCI_INTERRUPT_PIN] = 0x01;

	memory_region_init_io(&s->bar0, OBJECT(s), &b0_ops, s,
			      "vdci-b0", VDCI_BAR0_SIZE);
	pci_register_bar(pci_dev, 0,
			 PCI_BASE_ADDRESS_SPACE_MEMORY, &s->bar0);
	memory_region_init_io(&s->bar1, OBJECT(s), &b0_ops, s,
			      "vdci-b1", VDCI_BAR1_SIZE);
	pci_register_bar(pci_dev, 1,
			 PCI_BASE_ADDRESS_SPACE_IO, &s->bar1);
}

static void usb_vdci_pci_exit(PCIDevice *dev)
{
	printf("Starting exit...\n");
}

static void usb_vdci_pci_reset(DeviceState *dev)
{
	printf("Starting reset...\n");
}

static uint32_t usb_vdci_read_config(PCIDevice *pci_dev,
				       uint32_t address, int len)
{
	return pci_default_read_config(pci_dev, address, len);
}

static void usb_vdci_write_config(PCIDevice *pci_dev, uint32_t address,
				uint32_t val, int len)
{
	pci_default_write_config(pci_dev, address, val, len);
}

static const VMStateDescription vmstate_vdci = {
    .name = "vdci",
    .version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_PCIE_DEVICE(parent_obj, VDCIState),
        VMSTATE_MSIX(parent_obj, VDCIState),

        VMSTATE_END_OF_LIST()
    }
};

static void usb_vdci_class_init(ObjectClass *class, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(class);
	PCIDeviceClass *c = PCI_DEVICE_CLASS(class);
	c->realize = usb_vdci_pci_realize;
	c->exit = usb_vdci_pci_exit;
	c->vendor_id = 0x1500;
	c->device_id = 0x3500;
	c->revision = 0x01;
	c->class_id = PCI_CLASS_OTHERS;
	c->subsystem_vendor_id = 0x1500;
	c->subsystem_id = 0x3500;
	c->config_write = usb_vdci_write_config;
	c->config_read = usb_vdci_read_config;
	dc->desc = "USB Virtual Device Controller Interface device v1";
	dc->reset = usb_vdci_pci_reset;
	dc->vmsd = &vmstate_vdci;
	dc->props = NULL;
	set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
}

static const TypeInfo usb_vdci_info = {
	.name = TYPE_VDCI,
	.parent = TYPE_PCI_DEVICE,
	.instance_size = sizeof(VDCIState),
	.class_init = usb_vdci_class_init,
};

static void usb_vdci_register_types(void)
{
	printf("usb_vdci_register_types called...\n");
	type_register_static(&usb_vdci_info);
}
type_init(usb_vdci_register_types)

