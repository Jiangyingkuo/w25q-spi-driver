#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/mod_devicetable.h>


static int w25q_probe(struct spi_device *spi)
{
	printk(KERN_INFO "w25q probe called\n");
	return 0;
}
static int w25q_remove(struct spi_device *spi)
{
	printk(KERN_INFO "w25q remove called\n");
	return 0;
}

static const struct of_device_id w25q_of_match[] ={
	{.compatible = "demo,w25qxx"},
	{}
};

/*TODO: enable module autoloading MODULE_DEVICE_TABLE(of, w25q_of_match);*/

/*review structure initialize*/
static struct spi_driver w25q_driver = {
	.driver= {
		.name ="w25qxx",
		.owner=THIS_MODULE,
		.of_match_table = w25q_of_match, 
	},
	.probe =w25q_probe,
	.remove=w25q_remove,
	
};

static int __init w25q_init(void)
{
	return spi_register_driver(&w25q_driver);
}
static void __exit w25q_exit(void)
{
	spi_unregister_driver(&w25q_driver);
}
module_init(w25q_init);
module_exit(w25q_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple NOR FLASH SPI client driver");