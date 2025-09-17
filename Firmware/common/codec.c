/*
 * codec.h - codec I2C control port driver for WM8731
 * 12-06-20 E. Brombaugh
 */
/**
  * @brief  Initializes the audio codec and all related interfaces (control
  *         interface: I2C and audio interface: I2S)
  * @param  Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param  AudioFreq: Audio frequency used to play the audio stream.
  * @retval 0 if correct communication, else wrong communication
  */

#include "codec.h"
#include "shared_i2c.h"
#include "printf.h"

/* Stuff needed for CS4270 codec */

/* The 7 bits Codec address (sent through I2C interface) */
#define W8731_ADDR_0 0x1A

#define W8731_NUM_REGS 10

const uint16_t w8731_init_data[W8731_NUM_REGS] =
{
	0x017,			// Reg 00: Left Line In (0dB, mute off)
	0x017,			// Reg 01: Right Line In (0dB, mute off)
	0x079,			// Reg 02: Left Headphone out (0dB)
	0x079,			// Reg 03: Right Headphone out (0dB)
	0x012,			// Reg 04: Analog Audio Path Control (DAC sel, Mute Mic)
	0x008,			// Reg 05: Digital Audio Path Control (muste)
	0x060,			// Reg 06: Power Down Control (Clkout, Osc, Mic Off)
	0x002,			// Reg 07: Digital Audio Interface Format (msb, 16-bit, slave, I2S)
	0x000,			// Reg 08: Sampling Control (Normal, 256x, 48k ADC/DAC)
	0x001			// Reg 09: Active Control
};

enum wm8731_regs
{
	REG_LLIN,
	REG_RLIN,
	REG_LHP,
	REG_RHP,
	REG_APATH,
	REG_DPATH,
	REG_PCTL,
	REG_DAIF,
	REG_SMPL,
	REG_ACT,
	REG_RST = 0x0f
};

uint16_t w8731_shadow[W8731_NUM_REGS];

/*
 * Do all hardware setup for Codec except for final reset & config
 */
void Codec_Init(void)
{
}

/**
  * @brief  Writes a Byte to a given register into the WM8731 audio codec
			through the control interface (I2C)
  * @param  RegisterAddr: The address (location) of the register to be written.
  * @param  RegisterValue: the 9-bit value to be written into destination register.
  * @retval 0 if correct communication, else wrong communication
  */
HAL_StatusTypeDef Codec_WriteRegister(uint16_t DevAddr, uint8_t RegisterAddr,
	uint16_t RegisterValue)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t i2c_msg[2];

	/* update shadow regs */
	if(RegisterAddr<W8731_NUM_REGS)
		w8731_shadow[RegisterAddr] = RegisterValue;

	/* Assemble 2-byte data in WM8731 format */
    i2c_msg[0] = ((RegisterAddr<<1)&0xFE) | ((RegisterValue>>8)&0x01);
	i2c_msg[1] = RegisterValue&0xFF;

	status = HAL_I2C_Master_Transmit(&i2c_handler, DevAddr, i2c_msg, 2, 100);

	/* Check the communication status */
	if(status != HAL_OK)
	{
		/* Reset the I2C communication bus */
		shared_i2c_reset();

		printf("Codec_WriteRegister (WM8731): write to DevAddr 0x%02X / RegisterAddr 0x%02X failed - resetting.\n\r",
			DevAddr, RegisterAddr);
	}
	else
		printf("Codec_WriteRegister (WM8731): write to DevAddr 0x%02X / RegisterAddr 0x%02X = 0x%03X\n\r",
			DevAddr, RegisterAddr, RegisterValue);

	return status;
}

/**
  * @brief  Resets the audio codec. It restores the default configuration of the
  *         codec (this function shall be called before initializing the codec).
  * @note   This function calls an external driver function: The IO Expander driver.
  * @param  None
  * @retval None
  */
int32_t Codec_Reset(void)
{
	int32_t result = 0;
	uint8_t i;

    /* initial reset */
    Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_RST, 0);

	/* Load default values */
	for(i=0;i<W8731_NUM_REGS;i++)
	{
		if(Codec_WriteRegister(((W8731_ADDR_0)<<1), i, w8731_init_data[i]) != HAL_OK)
            result++;
	}

	return result;
}

/*
 * mute/unmute the codec outputs
 */
void Codec_Mute(uint8_t enable)
{
	uint8_t mute = enable ? 0x08 : 0x00;

	/* send mute cmd */
	Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_DPATH, mute);

	/* wait a bit for it to complete */
	HAL_Delay(20);
}

/*
 * set codec headphone volume
 */
void Codec_HPVol(uint8_t vol)
{
	/* set both chls volume */
	Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_LHP, 0x180 | (vol & 0x7f));
}

/*
 * set codec Input Source
 */
void Codec_InSrc(uint8_t src)
{
	uint16_t temp_reg;

	/* set line or mic input */
	temp_reg = (w8731_shadow[REG_APATH] & 0x1F9) | (src ? 0x04 : 0x02);
	Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_APATH, temp_reg);

	/* set line/mic power */
	//temp_reg = (w8731_shadow[REG_PCTL] & 0x1FC) | (src ? 0x01 : 0x02);
	//Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_PCTL, temp_reg);
}

/*
 * set codec Input volume
 */
void Codec_InVol(uint8_t vol)
{
	/* set both chls volume */
	Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_LLIN, 0x100 | (vol & 0x3f));
}

/*
 * set codec Mic Boost
 */
void Codec_MicBoost(uint8_t boost)
{
	uint16_t temp_reg;

	/* set/clear mic boost bit */
	temp_reg = (w8731_shadow[REG_APATH] & 0x1FE) | (boost ? 1 : 0);
	Codec_WriteRegister(((W8731_ADDR_0)<<1), REG_APATH, temp_reg);
}
