#include "I2C_SLAVE.h"
static void recdata_deal(BdeviceI2C *base);
#define ACK base->sda_pp();base->sda_l();base->sda_in();	
uint8_t FLAG_SEND = 0;
uint8_t FLAG_STOP = 0;
void scl_rising_interhandle(BdeviceI2C *base)//SCL上升沿
{
	switch(base->i2c_sta)
	{
		case DEVADDR:
			base->databitcount++;
			if(base->databitcount%9 != 0){
				base->datatemp <<= 1;
				base->datatemp |= base->sda_sta();
			}
			if(8 == base->databitcount){//读取完设备地址


				base->rec_devaddr = base->datatemp>>1;
 #ifndef debug
				if(base->devaddr != base->rec_devaddr){
					// 若不是本设备地址
					base->i2c_sta = IDLE_STOP;
					base->databitcount = 0;
					base->databittranscount = 0;
				}
#endif 
				base->RW_HL = (base->datatemp&0x01)?READ:WRITE;
				base->sclfall_sta = 1;//第8 SCL上升沿，ACK上升沿之前的沿,就是RW bit位所在沿
			}
			if(9 == base->databitcount){//回传ack，sda已经在scl下降沿拉低
				if(base->RW_HL == READ){
					FLAG_SEND = 1;
					base->i2c_sta = DATA_SEND;
				}else{
					base->sda_in();//
					base->i2c_sta = REGADDR;
				}
				base->databittranscount = 0;
				base->databitcount = 0;
			}
		break;
		case REGADDR:
			{
				base->databitcount++;
				if((base->databitcount %9 ) != 0){
					base->datatemp <<=1;
					base->datatemp |= base->sda_sta();
					if((base->databitcount %8) == 0){
						base->sclfall_sta = 1;//ack应该在这个沿之后的下降沿改变
					}
				}else{
					
					if(9 == base->databitcount){
						base->masreg_addr = base->datatemp;
					}else{// 18
						base->masreg_addr <<= 8;
						base->masreg_addr |= base->datatemp;
						base->i2c_sta = DATA;//有分支，若i2c读数据时候，会发送
						base->databitcount = 0;
					}
					if(1 == base->regaddr_width){
						base->i2c_sta = DATA;//有分支，若i2c读数据时候，会发送
						base->databitcount = 0;
					}
				}
			}
		break;
		case DATA:
		{
			uint8_t temp = 0;
			base->databitcount ++;
			temp = base->databitcount %9;
 
			if(temp != 0){
				base->datatemp <<=1;
				base->datatemp |= (0x01 & base->sda_sta());
 
				if(temp == 8){ 
					base->sclfall_sta = 1;//ack应该在这个沿之后的下降沿改变
				}				
			}else{//第九沿
				i2cslave_setdata(base,base->masreg_addr+((base->databitcount/9)-1),base->datatemp);
			}
		}
		break;
		// case DATA_SEND://读需要在下降沿准备数据
	}
}
void scl_falling_interhandle(BdeviceI2C *base)
{
	if(DATA_SEND == base->i2c_sta){//发送完数据后如何得到STOP信号??? 方法1：立即变成输入中断模式
		if(base->databittranscount %9 == 0){//0 9 ACK 下降沿
			base->sda_pp();//主机读从机
			i2cslave_getdata(base,base->masreg_addr+(base->databittranscount/9),(uint8_t*)&(base->datatemp));
		}// 0 1 2 3 4 5 6 7 8 9 
		// 0：ack后的下降沿
		if((base->databittranscount %9) != 8){
			((base->datatemp&0x80)?base->sda_h:base->sda_l)();//在scl为低时，输出数据
			base->datatemp <<= 1;
		}else{
			// 8
			base->sclfall_sta = 1;//第九下降沿，ACK上升沿之前的沿，
			// 这里可能会继续输出数据，也可能得到stop信号
		}
		base->databittranscount ++;		
	}
	if(1 == base->sclfall_sta)
	{
		base->sclfall_sta = 0;
		ACK;
	}
}
void sda_falling_interhandle(BdeviceI2C *base)
{
	// if((base->i2c_sta == IDLE_STOP) && (base->scl_sta() == 1))
	if(base->scl_sta() == 1)//
	{
		base->i2c_sta = DEVADDR;
		base->databitcount = 0;
		base->databittranscount = 0;
		base->datatemp = 0;
		//收到START_DEVADDR信号
	}
}
void sda_rising_interhandle(BdeviceI2C *base)
{
	if(base->scl_sta() == 1)
	{
		base->i2c_sta = IDLE_STOP;

		base->databitcount = 0;
		base->databittranscount = 0;
		base->datatemp = 0;
		base->sda_in();

		FLAG_STOP = 1;
	}
}
 
void i2cslave_init(BdeviceI2C *base,
	uint32_t (*SCL_STA)(void),//读取电平，高电平返回1,低电平返回0
	uint32_t (*SDA_STA)(void),//读取电平，高电平返回1,低电平返回0
	void (*SDA_IN)(void),//
	void (*SDA_OD)(void),//开漏输出
	void (*SDA_L)(void),
	void (*SDA_H)(void),
	uint8_t regaddr_width,//从设备的寄存器宽度
	uint8_t devaddr)
{
	base->scl_sta = SCL_STA;
	base->sda_sta = SDA_STA;
	base->sda_in = SDA_IN;
	base->sda_pp = SDA_OD;
	base->sda_l = SDA_L;
	base->sda_h = SDA_H;
 
	base->i2c_sta = IDLE_STOP;
	base->sclfall_sta = 0;
	base->databitcount = 0;
	base->regaddr_width = regaddr_width;
	
	base->databittranscount = 0;
	base->devaddr = devaddr;

}
 
/**********************************************************************************************************
*	函 数 名: int i2cslave_setdata(BdeviceI2C *base,uint16_t reg_addr,uint8_t data)
*	功能说明: 设置从i2c的内部寄存器
*	传    参: 
*	返 回 值: 0,设置成功，1:没有空间设置了，需要增加SLAVE_REG_NUM大小
*   说    明: 
*********************************************************************************************************/
int i2cslave_setdata(BdeviceI2C *base,uint16_t reg_addr,uint8_t data){
	uint8_t i_u8 = 0;
	for(i_u8 = 0;i_u8 < SLAVE_REG_NUM;i_u8++){
		if(base->slave_reg_addr[i_u8] == reg_addr){//曾经设置过这个寄存器
			base->slave_reg_data[i_u8] = data;//设置数据
			return 0;
		}
	}
	if(base->slave_reg_index < SLAVE_REG_NUM){
		// 还有空间设置
		base->slave_reg_addr[base->slave_reg_index] = reg_addr;
		base->slave_reg_data[base->slave_reg_index] = data;//设置数据
		base->slave_reg_index ++;
		return 0;
	}
	return 1;
}
/**********************************************************************************************************
*	函 数 名: int i2cslave_getdata(BdeviceI2C *base,uint16_t reg_addr,uint8_t *data)
*	功能说明: 获取从i2c的内部的数据
*	传    参: 
*	返 回 值: 0:成功获取，1：获取失败
*   说    明: 
*********************************************************************************************************/
int i2cslave_getdata(BdeviceI2C *base,uint16_t reg_addr,uint8_t *data){
	uint8_t i_u8 = 0;
	for(i_u8 = 0;i_u8 < SLAVE_REG_NUM;i_u8++){
		if(base->slave_reg_addr[i_u8] == reg_addr){//曾经设置过这个寄存器
			*data = base->slave_reg_data[i_u8];
			return 0;
		}
	}
	*data = 0;
	return 1;	
}
/**********************************************************************************************************
*	函 数 名: int i2cslave_isbusy(BdeviceI2C *base){
*	功能说明: 判断从i2c是否处于主i2c操作中
*	传    参: 
*	返 回 值: 0:有主设备操作，1:没有主设备操作
*   说    明: 
*********************************************************************************************************/
int i2cslave_isbusy(BdeviceI2C *base){
	return (base->i2c_sta != IDLE_STOP)?0:1;
}
