/*
 * eeprom.h - generic I2C eeprom driver
 * 07-12-19 E. Brombaugh
 */
 
 
#include "eeprom.h"
#include "shared_i2c.h"
#include "printf.h"

#define EEPROM_ADDRESS (0x50<<1)
#define EEPROM_MAX_TRIALS ((uint32_t)3000)
#define EEPROM_TIMEOUT 1000

#define EE_ADDR_MAGIC 0
#define EE_LEN_MAGIC 4
#define EE_MAGIC_VAL 0xbd722f00 
#define EEPROM_ADDR_SZ I2C_MEMADD_SIZE_16BIT
//#define EEPROM_ADDR_SZ I2C_MEMADD_SIZE_8BIT

/*
 * Check if the I2C bus is up and EEPROM is available 
 */
HAL_StatusTypeDef eeprom_CheckReady(void)
{
	return HAL_I2C_IsDeviceReady(&i2c_handler, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, EEPROM_TIMEOUT);
}

/*
 * Write a bunch of bytes
 */
HAL_StatusTypeDef eeprom_WriteBuff(uint16_t WriteAddr, uint8_t *Data, uint16_t sz)
{
	HAL_StatusTypeDef result;
	
    /* wait for setting write complete  or error */
    while((result = eeprom_CheckReady())==HAL_BUSY)
	{
		/* nothing */
	}
    
	if(result==HAL_OK)
	{
		//printf("eeprom_WriteBuff: i2c_handler->Instance = 0x%08X\n\r", i2c_handler.Instance);
		printf("eeprom_WriteBuff: 0x%08X 0x%08X 0x%04X\n\r", WriteAddr, Data, sz);
		return HAL_I2C_Mem_Write(&i2c_handler, EEPROM_ADDRESS, WriteAddr, EEPROM_ADDR_SZ, Data, sz, EEPROM_TIMEOUT);
	}
	
	/* error in ready check */
	printf("eeprom_WriteBuff: error during ready check\n\r");
	return result;
}

/*
 * Read a bunch of bytes
 */
HAL_StatusTypeDef eeprom_ReadBuff(uint16_t ReadAddr, uint8_t *Data, uint16_t sz)
{	
	HAL_StatusTypeDef result;
	
    /* wait for setting write complete  or error */
    while((result = eeprom_CheckReady())==HAL_BUSY)
	{
		/* nothing */
	}
    
	if(result==HAL_OK)
	{
	    printf("eeprom_ReadBuff: 0x%08X 0x%08X 0x%04X\n\r", ReadAddr, Data, sz);
		return HAL_I2C_Mem_Read(&i2c_handler, EEPROM_ADDRESS, ReadAddr, EEPROM_ADDR_SZ, Data, sz, EEPROM_TIMEOUT);
	}
	
	/* error in ready check */
	printf("eeprom_ReadBuff: error during ready check\n\r");
	return result;
}

/*
 * clear EEPROM to default state
 */
void eeprom_clear(void)
{
    uint32_t magic;
	
    /* magic */
    printf("eeprom_clear: initializing magic\n\r");
    magic = EE_MAGIC_VAL;
    while(eeprom_CheckReady()!=HAL_OK);
    eeprom_WriteBuff(EE_ADDR_MAGIC, (uint8_t *)&magic, EE_LEN_MAGIC);
}

HAL_StatusTypeDef eeprom_init(void)
{
	HAL_StatusTypeDef result;
    uint32_t magic;
	
    /* check if good settings in EEPROM */
    if((result = eeprom_ReadBuff(EE_ADDR_MAGIC, (uint8_t *)&magic, EE_LEN_MAGIC)) == HAL_OK)
	{
		printf("EEPROM Magic = 0x%8X\n\r", magic);
		if(magic != EE_MAGIC_VAL)
		{
			/* initialize EEPROM */
			eeprom_clear();
			
#if 1
			/* wait a bit and reset */
			printf("EEPROM defaults written - resetting\n\r");
			HAL_Delay(100);
			NVIC_SystemReset();
#endif
		}
	}
	else
	{
		printf("EEPROM init failed\n\r");
	}
	
	return result;
}
