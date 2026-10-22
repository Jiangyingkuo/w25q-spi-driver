#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define PAGE_PROGRAM_CMD 0x02
#define STAT_REG1 0X05
#define ERASE_SEC_CMD 0X20
#define TEST_ERASE	1
#define TEST_PAGE_PROG  1

static u8 rd_cmd =0x03;

//Read JEDEC ID
static int Read_ID(struct spi_device *spi)
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
// Write Enable (06h)
static int write_en(struct spi_device *spi)
{
	u8 cmd =0x06;
	int ret;
	struct spi_transfer t ={0};
	struct spi_message m;
	spi_message_init(&m);
	t.tx_buf =&cmd;
	t.len =sizeof(cmd);
	spi_message_add_tail(&t,&m);
	
	ret =spi_sync(spi,&m);
	if(ret <0)
		printk(KERN_ERR"write enable failed:%d\n",ret);
	return ret;
}

//read status register 1/2/3
static int rd_state(struct spi_device *spi,const u8 cmd, u8 *val)
{
	int ret;
	struct spi_transfer t[2];
	struct spi_message m;
	spi_message_init(&m);
	memset(t,0,sizeof(t));
	t[0].tx_buf =&cmd;
	t[0].len=sizeof(cmd);
	spi_message_add_tail(&t[0],&m);

	t[1].rx_buf =val;
	t[1].len =sizeof(*val);
	spi_message_add_tail(&t[1],&m);

	ret =spi_sync(spi,&m);
	if(ret <0 ) 
		printk(KERN_ERR"read status register failed: %d\n",ret);
	return ret;
}
/*wait_ready
poll status register 1,bit 0 (BUSY) until the chip is ready or timeout
TODO: use jiffies for the timeout instead of counting loops.*/
static int wait_ready(struct spi_device *spi,int timeout)
{
	u8 raw_status,bit0=0x01;
	int ret,i;
	for(i=0;i < timeout;i++){
		ret=rd_state(spi,STAT_REG1, &raw_status);
		if (ret) return ret;

		/*check status register bit 0*/
		if (!(raw_status & bit0)) return 0;
		msleep(1);
	}
	printk(KERN_INFO "wait ready timeout\n");
	return -ETIMEDOUT;
}
//read data(0x03)
static int rd_data(struct spi_device *spi,int addr,int len,u8 *buf)
{
	int ret;
	u8 cmd_addr[4];
	struct spi_transfer t[2];
	struct spi_message m;
	spi_message_init(&m);
	memset(t,0,sizeof(t));

	cmd_addr[0]=rd_cmd;
	cmd_addr[1] =(u8)(addr >>16);
	cmd_addr[2] =(u8)(addr >>8);
	cmd_addr[3] =(u8)(addr);

	t[0].tx_buf = cmd_addr;
	t[0].len =sizeof(cmd_addr);
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf =buf;
	t[1].len =len;
	spi_message_add_tail(&t[1],&m);

	ret =spi_sync(spi,&m);
	if(ret)
		printk(KERN_ERR"read data failed:%d\n",ret);
	return ret;
}
//sector erase (20h)
static int erase_sec(struct spi_device *spi,int addr)
{
	int ret;
	u8 cmd_addr[4];
	struct spi_transfer t;
	struct spi_message m;
	spi_message_init(&m);
	memset(&t,0,sizeof(t));

	cmd_addr[0]=ERASE_SEC_CMD;
	cmd_addr[1] =(u8)(addr >>16);
	cmd_addr[2] =(u8)(addr >>8);
	cmd_addr[3] =(u8)(addr);

	t.tx_buf =cmd_addr;
	t.len =sizeof(cmd_addr);
	spi_message_add_tail(&t,&m);

	/*write enable before page program*/
	ret=write_en(spi);
	if(ret) return ret;

	ret =spi_sync(spi,&m);
	if(ret) {
		printk(KERN_ERR"sector erase failed:%d\n",ret);
		return ret;
	}
	ret=wait_ready(spi,500);
	return ret;
}

//page program(02h)
static int page_pgm(struct spi_device *spi,int addr,int len,const u8 *buf)
{
	int ret;
	u8 cmd_addr[4];
	struct spi_transfer t[2];
	struct spi_message m;
	spi_message_init(&m);
	memset(t,0,sizeof(t));

	/* check if write beyond page boundary
	addr + len - 1 :page number of the last byte written*/
	if (len <= 0 || (addr >> 8) != ((addr + len - 1) >> 8))
		return -EINVAL;

	cmd_addr[0]=PAGE_PROGRAM_CMD;
	cmd_addr[1]=(u8)(addr >>16);
	cmd_addr[2]=(u8)(addr >>8);
	cmd_addr[3]=(u8)addr;

	t[0].tx_buf=cmd_addr;
	t[0].len=sizeof(cmd_addr);
	spi_message_add_tail(&t[0],&m);

	t[1].tx_buf=buf;
	t[1].len=len;
	spi_message_add_tail(&t[1],&m);

	/*write enable before page program*/
	ret=write_en(spi);
	if(ret) return ret;

	ret =spi_sync(spi, &m);
	if(ret) {
		printk(KERN_ERR"page program failed %d\n",ret);
		return ret;
	}

	//msleep(5); /* TODO: replace with wait_ready() */
	ret=wait_ready(spi,20);
	return ret;
}

static int w25q_probe(struct spi_device *spi)
{
	int ret,len=251;
	u8 raw_val;
	u8 *data_buff,data[30]={1,3,5,7,9,30,52,100,151,37,65,90,0};

	printk(KERN_INFO "w25q probe called\n");

	ret =Read_ID(spi);
	if (ret) return ret;

	ret=rd_state(spi,STAT_REG1, &raw_val);
	if (ret) return ret;
	printk(KERN_INFO "WEL is : %d\n",(raw_val >> 1)& 1 );

	ret=rd_state(spi,STAT_REG1, &raw_val);
	if (ret) return ret;
	if(raw_val & (1 <<1)){
		printk(KERN_INFO"WEL is set 1 \n");
	}
	data_buff =(u8 *)kmalloc(len,GFP_KERNEL);
	if (data_buff ==NULL)
		return -ENOMEM;
	ret=rd_data(spi,0x000005,len,data_buff);
	if (!ret)
		print_hex_dump(KERN_INFO, "w25q: ", DUMP_PREFIX_OFFSET, 16, 1,data_buff, len, false);

#if TEST_ERASE
	printk("TEST ERASE SECTOR\n");
	//erase sector , address: 0x000000
	ret=erase_sec(spi,0x000000);
	if(ret)
		goto err_free;

	memset(data_buff,0,len);
	ret=rd_data(spi,0x000005,len,data_buff);
	if (!ret)
		print_hex_dump(KERN_INFO, "w25q: ", DUMP_PREFIX_OFFSET, 16, 1,data_buff, len, false);
#endif
#if TEST_PAGE_PROG
	printk("TEST PAGE PROGRAM\n");
	memcpy(data_buff, data,sizeof(data));
	ret =page_pgm(spi,0x00000,30,data_buff);
	if (ret) goto err_free;

	memset(data_buff, 0, len);
	ret = rd_data(spi, 0x000000, sizeof(data), data_buff);
	if (ret)
		goto err_free;

	print_hex_dump(KERN_INFO, "w25q: ", DUMP_PREFIX_OFFSET, 16, 1,
		       data_buff, sizeof(data), false);
	if (memcmp(data_buff, data, sizeof(data)))
		printk(KERN_ERR "page program verify failed\n");
	else
		printk(KERN_INFO "page program verify ok\n");
#endif
	err_free:
		kfree(data_buff);
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