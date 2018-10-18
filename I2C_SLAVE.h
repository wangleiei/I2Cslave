#ifndef __i2cslave
#define __i2cslave 
#include "stm8l15x.h"

typedef enum {DATA_SEND,IDLE_STOP,DEVADDR,REGADDR,REGADDRWAIT,DEVADDR_ACK,DATA}I2C_SLAVESTA;
typedef enum {READ = 1,WRITE = 0}RW_STA;
typedef uint32_t (*SCL_STA)(void);
typedef uint32_t (*SDA_STA)(void);
typedef void (*SDA_IN)(void);
typedef void (*SDA_OD)(void);
typedef void (*SDA_L)(void);
typedef void (*SDA_H)(void);
#define SLAVE_REG_NUM 10//作为从机使用时，从机的内部寄存器个数

typedef struct BdeviceI2C
{
	SCL_STA scl_sta;
	SDA_STA sda_sta;
	SDA_IN  sda_in;
	SDA_OD  sda_pp;
	SDA_L   sda_l;
	SDA_H   sda_h;
	I2C_SLAVESTA i2c_sta;
	uint8_t sclfall_sta;

	uint8_t rec_devaddr;//收到的主机地址
	uint8_t devaddr;//本设备地址

	uint16_t masreg_addr;//主机发出的从机地址
	uint8_t regaddr_width;//地址宽度

	uint8_t databitcount;//

	uint8_t databittranscount;//

	uint8_t datatemp;

	RW_STA RW_HL;//用于操作是写入还是读出

	uint16_t slave_reg_addr[SLAVE_REG_NUM];
	uint8_t slave_reg_data[SLAVE_REG_NUM];
	uint8_t slave_reg_index;

	uint8_t read_reg_index;//读取数据时用来计数第几个数据
	// uint8_t scl_rising_intercount;
	// uint8_t scl_falling_intercount;
	// uint8_t sda_falling_intercount;
	// uint8_t sda_rising_intercount;
	// uint8_t start_count;
	// uint8_t stop_count;
	// uint8_t dev_count;
	// uint8_t regaddr_count;
	// I2C_SLAVESTA stat[10]
}BdeviceI2C;

void scl_rising_interhandle(BdeviceI2C *base);
void scl_falling_interhandle(BdeviceI2C *base);
void sda_falling_interhandle(BdeviceI2C *base);
void sda_rising_interhandle(BdeviceI2C *base);
// @DateTime:    2018-10-18 15:53:09
// @Author wll 实测能处理800hz速率的i2c stm8l 内部16MHZ晶振频率
void i2cslave_init(BdeviceI2C *base,
	uint32_t (*SCL_STA)(void),//读取电平，高电平返回1,低电平返回0
	uint32_t (*SDA_STA)(void),//读取电平，高电平返回1,低电平返回0
	void (*SDA_IN)(void),//
	void (*SDA_OD)(void),//开漏输出
	void (*SDA_L)(void),
	void (*SDA_H)(void),
	uint8_t regaddr_width,//从设备的寄存器宽度
	uint8_t devaddr);//从设备的设备地址

int i2cslave_setdata(BdeviceI2C *base,uint16_t reg_addr,uint8_t data);
int i2cslave_getdata(BdeviceI2C *base,uint16_t reg_addr,uint8_t *data);
int i2cslave_isbusy(BdeviceI2C *base);

#endif