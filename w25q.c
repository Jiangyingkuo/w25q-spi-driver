#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/mod_devicetable.h>

//Read JEDEC ID
int Read_ID(struct spi_device *spi)
{
	u8 cmd =0x9F;
	int ret;
	char rd_buf[3] ={0};
	struct spi_transfer t[2];
	struct spi_message m;
	spi_message_init(&m);
	memset(t,0,sizeof(t));
	t[0].tx_buf = &cmd;
	t[0].len =sizeof(cmd);
	spi_message_add_tail(&t[0],&m);

	t[1].rx_buf = rd_buf;
	t[1].len =3;
	spi_message_add_tail(&t[1],&m);

	ret =spi_sync(spi,&m);
	if(ret < 0) {
		printk(KERN_ERR "read id error %d \n",ret);
		return ret;
	}
	printk(KERN_INFO "%x, %x, %x \n", rd_buf[0],rd_buf[1],rd_buf[2]);
	return 0;
}

static int w25q_probe(struct spi_device *spi)
{
	int ret;
	printk(KERN_INFO "w25q probe called\n");
	ret =Read_ID(spi);
	return ret;
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