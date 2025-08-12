/* ========================================
 *
 *	i2cdevice.cpp
 *		description: TouchMidi I2C Device Driver
 *
 *	Copyright(c)2018- Masahiko Hasebe at Kigakudoh
 *  This software is released under the MIT License, see LICENSE.txt
 *
 * ========================================
*/
#include	"Arduino.h"
#include  <Wire.h>
#include  <avr/pgmspace.h>

#include  "constants.h"
#include	"peripheral.h"


//---------------------------------------------------------
//    Variables
//---------------------------------------------------------
int   i2cErrCode;

//---------------------------------------------------------
//		Initialize I2C Device
//---------------------------------------------------------
void wireBegin( void )
{
  Wire.setClock(400000);
  Wire.setSDA(6);
  Wire.setSCL(7);
	Wire.begin();
}
//---------------------------------------------------------
//		Write I2C Device
//    Err Code
//      0:success
//      1:data too long to fit in transmit buffer
//      2:received NACK on transmit of address
//      3:received NACK on transmit of data
//      4:other error
//---------------------------------------------------------
int write_i2cDevice( unsigned char adrs, unsigned char* buf, int count )
{
	Wire.beginTransmission(adrs);
  Wire.write(buf,count);
	return Wire.endTransmission();
}
//---------------------------------------------------------
//		Read 1byte I2C Device
//---------------------------------------------------------
int read1byte_i2cDevice( unsigned char adrs, unsigned char* wrBuf, unsigned char* rdBuf, int wrCount )
{
	unsigned char err;

	Wire.beginTransmission(adrs);
  Wire.write(wrBuf,wrCount);
	err = Wire.endTransmission(false);
	if ( err != 0 ){ return err; }

	err = Wire.requestFrom(adrs,(uint8_t)1,(uint8_t)0);
	while(Wire.available()) {
		*rdBuf = Wire.read();
	}

	//err = Wire.endTransmission(true);
	//return err;
  return 0;
}
//---------------------------------------------------------
//		Read N byte I2C Device
//---------------------------------------------------------
//送信結果 (byte) 
//0: 成功 
//1: 送ろうとしたデータが送信バッファのサイズを超えた 
//2: スレーブ・アドレスを送信し、NACKを受信した 
//3: データ・バイトを送信し、NACKを受信した 
//4: その他のエラー 
//
int read_nbyte_i2cDevice( unsigned char adrs, unsigned char* wrBuf, unsigned char* rdBuf, int wrCount, int rdCount )
{
	unsigned char err;

	Wire.beginTransmission(adrs);
  Wire.write(wrBuf,wrCount);
	err = Wire.endTransmission(false);
	if ( err != 0 ){ return err; }

	err = Wire.requestFrom(adrs,static_cast<uint8_t>(rdCount),(uint8_t)0);
	int rdAv = 0;
	while((rdAv = Wire.available()) != 0) {
		*(rdBuf+rdCount-rdAv) = Wire.read();
	}

	//err = Wire.endTransmission(true);
	//return err;

	return 0;
}
// ハングしてはいけない本番用
int read_nbyte_i2cDeviceX( unsigned char adrs, unsigned char* wrBuf, unsigned char* rdBuf, int wrCount, int rdCount )
{
	unsigned char err;
  volatile int cnt=0;

	Wire.beginTransmission(adrs);
  Wire.write(wrBuf,wrCount);
	err = Wire.endTransmission(false);
	if ( err != 0 ){ return err; }

	err = Wire.requestFrom(adrs,static_cast<uint8_t>(rdCount),(uint8_t)0);
	int rdAv = 0;
	while(((rdAv = Wire.available()) != 0) && (cnt < 10)){
		*(rdBuf+rdCount-rdAv) = Wire.read();
    cnt += 1;
	}

	return 0;
}
//---------------------------------------------------------
//    Read Only N byte I2C Device
//    Err Code
//      0:success
//      1:data too long to fit in transmit buffer
//      2:received NACK on transmit of address
//      3:received NACK on transmit of data
//      4:other error
//---------------------------------------------------------
int read_only_nbyte_i2cDevice( unsigned char adrs, unsigned char* rdBuf, int rdCount )
{
  unsigned char err;

  err = Wire.requestFrom(adrs,static_cast<uint8_t>(rdCount),static_cast<uint8_t>(false));
  int rdAv = Wire.available();
  while( rdAv ) {
    *(rdBuf+rdCount-rdAv) = Wire.read();
    rdAv--;
  }

  err = Wire.endTransmission(true);
  return err;
}


#ifdef USE_CY8CMBR3110
//-------------------------------------------------------------------------
//			Cap Sense CY8CMBR3110 (Touch Sencer : I2c Device)
//-------------------------------------------------------------------------
#define   MAX_DEVICE_MBR3110    MAX_KAMABOKO_NUM
#define		CONFIG_DATA_OFFSET	  0
#define		CONFIG_DATA_SZ			  128

#define		SENSOR_EN		          0x00	//	Register Address
#define		SENSITIVITY0			    0x08	//	Register Address
#define		SENSITIVITY1			    0x09	//	Register Address
#define		SENSITIVITY2			    0x0a	//	Register Address
#define		I2C_ADDR				      0x51	//	Register Address
#define		CONFIG_CRC				    0x7e	//	Register Address

#define		CTRL_CMD				      0x86	//	Register Address
#define		POWER_ON_AND_FINISHED	0x00
#define		SAVE_CHECK_CRC		    0x02
#define		DEVICE_RESET			    0xff

#define		CTRL_CMD_ERR			    0x89	//	Register Address

#define		FAMILY_ID_ADRS			  0x8f	//	Register Address
#define		FAMILY_ID				      0x9a
#define		DEVICE_ID_ADRS			  0x90	//	Register Address
#define		DEVICE_ID_LOW			    0x02
#define		DEVICE_ID_HIGH			  0x0a

#define		TOTAL_WORKING_SNS		  0x97	//	Register Address
#define		SNS_VDD_SHORT			    0x9a	//	Register Address
#define		SNS_GND_SHORT			    0x9c	//	Register Address
#define		BUTTON_STAT				    0xaa	//	Register Address

#define  SENSOR_ID              0x82  //  Register Address
#define  DEBUG_SENSOR_ID        0xdc  //  Register Address
#define  DEBUG_RAW_COUNT        0xe2  //  Register Address

static const unsigned char CAP_SENSE_ADDRESS_ORG = 0x37;  //  Factory-Set
static const unsigned char CAP_SENSE_ADDRESS_1 = 0x38;
static const unsigned char CAP_SENSE_ADDRESS_2 = 0x39;
static const unsigned char CAP_SENSE_ADDRESS_3 = 0x3a;
static const unsigned char CAP_SENSE_ADDRESS_4 = 0x3b;
static const unsigned char CAP_SENSE_ADDRESS_5 = 0x3c;
static const unsigned char CAP_SENSE_ADDRESS_6 = 0x3d;
static const unsigned char CAP_SENSE_ADDRESS_7 = 0x3e;
static const unsigned char CAP_SENSE_ADDRESS_8 = 0x3f;
static const unsigned char CAP_SENSE_ADDRESS_9 = 0x40;
static const unsigned char CAP_SENSE_ADDRESS_10 = 0x41;
static const unsigned char CAP_SENSE_ADDRESS_11 = 0x42;
static const unsigned char CAP_SENSE_ADDRESS_12 = 0x43;

/*----------------------------------------------------------------------------*/
//
//      Write CY8CMBR3110 Config Data
//
/*----------------------------------------------------------------------------*/
// wide range small resolution
static const unsigned char tCY8CMBR3110_ConfigData[][CONFIG_DATA_SZ] PROGMEM = {
{
/* Project: C:\Users\hasebems\Documents\Cypress Projects\Design0602\Design0602.cprj
 * Generated: 2019/06/02 6:52:53 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x38u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xB7u, 0xCAu
},
{
/* Project: C:\Users\hasebems\Documents\Cypress Projects\Design0602\Design0602-2.cprj
 * Generated: 2019/06/02 7:07:15 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x39u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x94u, 0x1Eu
},
{
/* Project: C:\Users\hasebems\Documents\Cypress Projects\Design0602\Design210124-3.cprj
 * Generated: 2021/01/24 22:53:50 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x3Au, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xD0u, 0x72u
},
{
/* Project: C:\Users\hasebems\Documents\Cypress Projects\Design0602\Design210124-4.cprj
 * Generated: 2021/01/24 22:54:37 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x3Bu, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xF3u, 0xA6u
},
{  //5
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:24:23 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x3Cu, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x58u, 0xAAu
},
{  //6
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:24:41 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x3Du, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x7Bu, 0x7Eu
},
{  //7
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:25:06 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x3Eu, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x3Fu, 0x12u
},
{  //8
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:25:35 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x3Fu, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Cu, 0xC6u
},
{  //9
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:25:51 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x40u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xD9u, 0xC1u
},
{  //10
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:26:07 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x41u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFAu, 0x15u
},
{  //11
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:27:18 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x42u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xBEu, 0x79u
},
{  //12
/* Project: C:\Users\Masahiko HASEBE\OneDrive - YAMAHA Group\prv\cy8cmbr3110\TouchKeyboard\TouchKeyboard.cprj
 * Generated: 2022/06/18 10:27:30 +09:00 */
    0xFFu, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0xFFu, 0xFFu, 0x0Fu, 0x00u, 0x80u, 0x80u, 0x80u, 0x80u,
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
    0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1Eu, 0x1Eu, 0x00u,
    0x00u, 0x1Eu, 0x1Eu, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
    0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x11u, 0x02u, 0x01u, 0x08u,
    0x00u, 0x43u, 0x01u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x9Du, 0xADu
}
};
//-------------------------------------------------------------------------
static const unsigned char tI2cAdrs[] =
{
  CAP_SENSE_ADDRESS_1,
  CAP_SENSE_ADDRESS_2,
  CAP_SENSE_ADDRESS_3,
  CAP_SENSE_ADDRESS_4,
  CAP_SENSE_ADDRESS_5,
  CAP_SENSE_ADDRESS_6,
  CAP_SENSE_ADDRESS_7,
  CAP_SENSE_ADDRESS_8,
  CAP_SENSE_ADDRESS_9,
  CAP_SENSE_ADDRESS_10,
  CAP_SENSE_ADDRESS_11,
  CAP_SENSE_ADDRESS_12
};
//-------------------------------------------------------------------------
void MBR3110_resetAll(int maxdevNum)
{
  unsigned char i2cdata[2] = {CTRL_CMD, POWER_ON_AND_FINISHED};

  if (maxdevNum >= MAX_DEVICE_MBR3110){ return;}

  delay(15);
  for (int i=0; i<maxdevNum; i++){
    write_i2cDevice(tI2cAdrs[i],i2cdata,2);
  }
  delay(900);
}
//-------------------------------------------------------------------------
void MBR3110_reset(unsigned char i2cAdrs)
{
  unsigned char i2cdata[2] = {CTRL_CMD, POWER_ON_AND_FINISHED};
  delay(15);
  write_i2cDevice(i2cAdrs,i2cdata,2);
  delay(900);
}
//-------------------------------------------------------------------------
int MBR3110_init( int number )
{
	unsigned char selfCheckResult;

  if (number >= MAX_DEVICE_MBR3110){ return -1;}

  const unsigned char* configData = tCY8CMBR3110_ConfigData[number];
  unsigned char i2cAdrs = tI2cAdrs[number];

  unsigned char checksum1 = pgm_read_byte(configData+126);
  unsigned char checksum2 = pgm_read_byte(configData+127);
  int err = MBR3110_checkWriteConfig(checksum1,checksum2,i2cAdrs);
  if ( err != 0 ){
    return err;
  }

  err = MBR3110_selfTest(&selfCheckResult,number);
  if ( err != 0 ){ return err; }

  if ( selfCheckResult & 0x80 ){
    err = selfCheckResult & 0x1f;  //  SENSOR_COUNT
    return err;
  }

  return err;
}
//-------------------------------------------------------------------------
int MBR3110_setup( int number )
{
  unsigned char i2cAdrs = tI2cAdrs[number];
  int err;

  if (number >= MAX_DEVICE_MBR3110){ return -1;}

  MBR3110_reset(i2cAdrs); 

  const unsigned char* configData = tCY8CMBR3110_ConfigData[number];
  const unsigned char checksum1 = pgm_read_byte(configData+126);
  const unsigned char checksum2 = pgm_read_byte(configData+127);       
  if ( MBR3110_checkWriteConfig(checksum1,checksum2,i2cAdrs) == 0 ){
    //  checksum got correct.
    //  it doesn't need to write config.
    return 0;
  }

  //  check if factory preset device
  err = MBR3110_writeConfig(number, CAP_SENSE_ADDRESS_ORG);
  if ( err != 0 ){
    //  if err then change I2C Address
    //    and rewrite config to current device
    err = MBR3110_writeConfig(number, i2cAdrs);
    if ( err != 0 ){ return err; }
  }

  //  After writing, Check again.
  MBR3110_reset(i2cAdrs);
  err = MBR3110_checkWriteConfig(checksum1,checksum2,i2cAdrs);
  if ( err != 0 ){
    //  checksum error
    return err;
  }

  unsigned char selfCheckResult;
  err = MBR3110_selfTest(&selfCheckResult,number);
  if ( err != 0 ){ return err; }

  if ( selfCheckResult & 0x80 ){
    err = selfCheckResult & 0x1f;  //  SENSOR_COUNT
    return err;
  }

  return err;
}
//-------------------------------------------------------------------------
int MBR3110_readData( unsigned char cmd, unsigned char* data, int length, unsigned char i2cAdrs )
{
	int err;
	volatile int cnt = 0;
	unsigned char wrtBuf = cmd;

	while(1) {
		err = read_nbyte_i2cDeviceX(i2cAdrs,&wrtBuf,data,1,length);
		if ( err == 0 ) break;
		if ( ++cnt > 10 ){
			return err;
		}
		delay(1);
	}
	return 0;
}
//-------------------------------------------------------------------------
int MBR3110_selfTest( unsigned char* result, int number )
{
	int err;

	err = MBR3110_readData(TOTAL_WORKING_SNS,result,1,tI2cAdrs[number]);
	if ( err ){ return err; }

	return 0;
}
//-------------------------------------------------------------------------
void MBR3110_changeSensitivity( unsigned char data, int number )		//	data : 0-3
{
	unsigned char i2cdata[2];
	unsigned char regData2 = data & 0x03;
	regData2 |= (regData2 << 2);
	unsigned char regData4 = regData2 | (regData2<<4);
  unsigned char i2cAdrs = tI2cAdrs[number];  

	i2cdata[0] = SENSITIVITY0;
	i2cdata[1] = regData4;
	if ( write_i2cDevice(i2cAdrs,i2cdata,2) ){
		i2cdata[0] = SENSITIVITY1;
		i2cdata[1] = regData4;
		if ( write_i2cDevice(i2cAdrs,i2cdata,2) ){
			i2cdata[0] = SENSITIVITY2;
			i2cdata[1] = regData2;
			if ( write_i2cDevice(i2cAdrs,i2cdata,2) ){
				return;
			}
		}
	}
}
//-------------------------------------------------------------------------
int MBR3110_readTouchSw( unsigned char* touchSw, int number )
{
  int err;

  err = MBR3110_readData(BUTTON_STAT,touchSw,2,tI2cAdrs[number]);
  if ( err ){ return err; }

  return 0;
}
//-------------------------------------------------------------------------
int MBR3110_checkWriteConfig( unsigned char checksumL, unsigned char checksumH, unsigned char crntI2cAdrs )
{
	unsigned char data[2];
	int err;
  int cnt = 0;

  while(cnt<100){
    err = MBR3110_readData(CONFIG_CRC,data,2,crntI2cAdrs);
    if ( err ){ return err; }

    //	err=0 means it's present config
    if (( data[0] == checksumL ) && ( data[1] == checksumH )){ return 0; }
    delay(1);
    ++cnt;
  }

	return -1;  //  check sum didn't match
}
//-------------------------------------------------------------------------
int MBR3110_writeConfig( int number, unsigned char crntI2cAdrs )
{
  int err;
	unsigned char	data[CONFIG_DATA_SZ+1];
  const unsigned char* configData = tCY8CMBR3110_ConfigData[number];

  //*** Prepare ***
  MBR3110_reset(crntI2cAdrs);

	//*** Step 1 ***
	//	Check Power On

	//	Check I2C Address
	err = MBR3110_readData(I2C_ADDR,data,1,crntI2cAdrs);
	if ( err != 0 ){ return err; }
	if ( data[0] != crntI2cAdrs ){ return -1; }

	//*** Step 2 ***
	err = MBR3110_readData(DEVICE_ID_ADRS,data,2,crntI2cAdrs);
	if ( err != 0 ){ return err; }
	if (( data[0] != DEVICE_ID_LOW ) || ( data[1] != DEVICE_ID_HIGH )){ return -2; }

	err = MBR3110_readData(FAMILY_ID_ADRS,data,1,crntI2cAdrs);
	if ( err != 0 ){ return err; }
	if ( data[0] != FAMILY_ID ){ return -3; }

	//*** Step 3 ***
	//	send Config Data
  for ( int i=0; i<CONFIG_DATA_SZ; i++ ){
    data[0] = CONFIG_DATA_OFFSET+i;
    data[1] = pgm_read_byte(configData+i);
    err = write_i2cDevice(crntI2cAdrs,data,2);
    if ( err != 0 ){ return err;}
  }

	//	Write to flash
	data[0] = CTRL_CMD;
	data[1] = SAVE_CHECK_CRC;
	err = write_i2cDevice(crntI2cAdrs,data,2);
	if ( err != 0 ){ return err;}

	//	220msec Wait
	delay(300);

	//	Check to finish writing
	unsigned char wrtData = CTRL_CMD_ERR;
	err = read1byte_i2cDevice(crntI2cAdrs,&wrtData,data,1);
	if ( data[0] == 0xfe ){ return -4;}       //  bad check sum
  else if ( data[0] == 0xff ){ return -5;}  //  invalid command
  else if ( data[0] == 0xfd ){ return -6;}  //  failed to write flash

	//	Reset
	data[0] = CTRL_CMD;
	data[1] = DEVICE_RESET;
	err = write_i2cDevice(crntI2cAdrs,data,2);
	if ( err != 0 ){ return err;}

	//	100msec Wait
  delay(100);

	//*** Step 4 ***
	//	Get Config Data
	wrtData = CONFIG_DATA_OFFSET;
	err =  read_nbyte_i2cDevice(tI2cAdrs[number],&wrtData,data,1,CONFIG_DATA_SZ);
	if ( err != 0 ){ return err; }

	//	Compare both Data
	for ( int i=0; i<CONFIG_DATA_SZ; i++ ){
		if ( pgm_read_byte(configData+i) != data[i] ){ return /*data[i]*/i; }
	}
	
	return 0;
}
//---------------------------------------------------------
int mbr3110_read_rawdata( unsigned int number, unsigned char (&read_data)[2] )
{
	unsigned char	data[2];
  data[0] = SENSOR_ID;
	data[1] = number;
  int err = write_i2cDevice(tI2cAdrs[number],data,2);
	if ( err != 0 ){ return err;}

	unsigned char wrtData = DEBUG_RAW_COUNT;
  err =  read_nbyte_i2cDevice(tI2cAdrs[number],&wrtData,read_data,1,2);
	if ( err != 0 ){ return err; }

  return 0;
}
#endif



//-------------------------------------------------------------------------
//			AT42QT1070
//-------------------------------------------------------------------------
#ifdef USE_AT42QT1070
static const uint8_t AT42QT_I2C_ADRS = 0x1B;
// read
static const uint8_t AT42QT_STATUS = 2;

static const uint8_t AT42QT_LP_MODE = 54;
static const uint8_t AT42QT_MAX_DUR = 55;

void AT42QT_init( void )
{
  uint8_t i2cdata[2] = {AT42QT_LP_MODE, 0};
  int err = write_i2cDevice(AT42QT_I2C_ADRS, i2cdata,2);
  if ( err != 0 ){ return; }

  i2cdata[0] = AT42QT_MAX_DUR;
  err = write_i2cDevice(AT42QT_I2C_ADRS, i2cdata, 2);
  if ( err != 0 ){ return; }
}

int AT42QT_read( size_t key, uint8_t (&rdraw)[2], bool ref )
{
  uint8_t wd = 0;
  if (ref) {
    wd = 18 + key*2;
  } else {
    wd = 4 + key*2;  //  0-6
  }  
  return read_nbyte_i2cDeviceX(AT42QT_I2C_ADRS, &wd, rdraw, 1, 2);
}
#endif


#ifdef USE_ADA88
//---------------------------------------------------------
//		<< ADA88 >>
//---------------------------------------------------------
static const unsigned char ADA88_I2C_ADRS = 0x70;
//---------------------------------------------------------
//		Initialize ADA88 LED Matrix
//---------------------------------------------------------
void ada88_init( void )
{
	unsigned char i2cBuf[2] = {0};
	i2cBuf[0] = 0x21;
	write_i2cDevice( ADA88_I2C_ADRS, i2cBuf, 2 );

	i2cBuf[0] = 0x81;
	write_i2cDevice( ADA88_I2C_ADRS, i2cBuf, 2 );

	i2cBuf[0] = 0xef;
	write_i2cDevice( ADA88_I2C_ADRS, i2cBuf, 2 );

	ada88_write(0);
}
//---------------------------------------------------------
//	write Character to Ada LED-matrix
//---------------------------------------------------------
void ada88_write( int letter )
{
	unsigned char i2cBufx[17];
	static const unsigned char letters[][8] PROGMEM = {

  //  2:B の場合          実際の表示(裏返して一つずれる)
  //  o___ _ooo : 0x87   oooo ____
  //  o___ o___ : 0x88   o___ o___
  //  o___ o___ : 0x88   o___ o___
  //  o___ _ooo : 0x87   oooo ____
  //  o___ o___ : 0x88   o___ o___
  //  o___ o___ : 0x88   o___ o___
  //  o___ o___ : 0x88   o___ o___
  //  o___ _ooo : 0x87   oooo ____

		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	//	0:nothing
		{0x02,0x05,0x88,0x88,0x8f,0x88,0x88,0x88},	//	1:A
		{0x87,0x88,0x88,0x87,0x88,0x88,0x88,0x87},	//	2:B
		{0x07,0x88,0x88,0x80,0x80,0x88,0x88,0x07},	//	3:C
		{0x87,0x88,0x88,0x88,0x88,0x88,0x88,0x87},	//	4:D
		{0x87,0x80,0x80,0x87,0x80,0x80,0x80,0x8f},	//	5:E
		{0x87,0x80,0x80,0x87,0x80,0x80,0x80,0x80},	//	6:F
		{0x07,0x88,0x80,0x80,0x8e,0x88,0x88,0x07},	//	7:G
		{0x88,0x88,0x88,0x8f,0x88,0x88,0x88,0x88},	//	8:H

		{0x02,0x05,0x88,0xe8,0xaf,0xe8,0xa8,0xa8},	//	9:AF
		{0x87,0x88,0x88,0xe7,0xa8,0xe8,0xa8,0xa7},	//	10:BF
		{0x07,0x88,0x88,0xe0,0xa0,0xe8,0xa8,0x27},	//	11:CF
		{0x87,0x88,0x88,0xe8,0xa8,0xe8,0xa8,0xa7},	//	12:DF
		{0x87,0x80,0x80,0xe7,0xa0,0xe0,0xa0,0xaf},	//	13:EF
		{0x87,0x80,0x80,0xe7,0xa0,0xe0,0xa0,0xa0},	//	14:FF
		{0x07,0x88,0x80,0xe0,0xae,0xe8,0xa8,0x27},	//	15:GF

		{0x97,0x90,0x90,0x97,0x90,0x90,0x90,0xd0},	//	16:Fl.
		{0x13,0x94,0x94,0xf4,0xd4,0xd4,0xb4,0x53},	//	17:Ob.

		{0x04,0x1f,0x04,0x1f,0x04,0x0f,0x15,0x22},	//	18:MA
		{0x04,0x0f,0x04,0x1e,0x08,0x12,0x01,0x0e},	//	19:KI
		{0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa},  //	20:
		{0x03,0x80,0x80,0x49,0x4a,0x4a,0xca,0x79},	//	21:SU(setup)
		{0x09,0x8a,0xca,0xaa,0x9a,0xaa,0xaa,0x49},	//	22:Ok
		{0x83,0x80,0x80,0xab,0xd8,0x88,0x88,0x8b},	//	23:Er
    {0x80, 0xf0, 0x90, 0x90, 0xf0, 0x90, 0x90, 0xf7}, //	24:LE(LED)
    {0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00}, //	25:--
    {0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff}, //	26: 枠大
    {0x00, 0x3f, 0x21, 0x21, 0x21, 0x21, 0x3f, 0x00}, //	27: 枠中
    {0x00, 0x00, 0x1e, 0x12, 0x12, 0x1e, 0x00, 0x00}, //	28: 枠小
    {0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x00}, //	29: 🔲
	};

	i2cBufx[0] = 0;
	for (int i=0; i<8; i++){
		i2cBufx[i*2+1] = pgm_read_byte(&(letters[letter][i]));
		i2cBufx[i*2+2] = 0;
	}
	write_i2cDevice( ADA88_I2C_ADRS, i2cBufx, 17 );
}
//---------------------------------------------------------
void ada88_writeBit( uint16_t num )	//	num 0x0000 - 0xffff
{
//  oooooooo : No Light
//  oooooooo : No Light
//  oooooooo : No Light
//  oooooooo : No Light
//  oooooooo : No Light
//  oooooooo : No Light
//  xxxxxxxx : Upper 8bit Display 
//  xxxxxxxx : Lower 8bit Display
//
	unsigned char i2cBufx[17];
	unsigned char ledPtn[8] = {0};

  //  upper 8bit
	uint8_t lsb = 0x00;
	uint8_t bits = static_cast<uint8_t>((num >> 8) & 0x00ff);
	if (bits & 0x01){lsb=0x80;}
	ledPtn[6] = (bits >> 1) + lsb; // upper

  //  lower 8bit
	bits = static_cast<uint8_t>(num & 0x00ff);
	lsb = 0x00;
	if (bits & 0x01){lsb=0x80;}
	ledPtn[7] = (bits >> 1) + lsb; // lower

  //  Set LED I2C Buffer
	i2cBufx[0] = 0;
	for ( int i=0; i<8; i++ ){
		i2cBufx[i*2+1] = ledPtn[i];
		i2cBufx[i*2+2] = 0;
	}
	write_i2cDevice( ADA88_I2C_ADRS, i2cBufx, 17 );
}
//---------------------------------------------------------
static const unsigned char numletter[10][5] PROGMEM = {//3*5
  { 0x07, 0x05, 0x05, 0x05, 0x07 }, // 0
  { 0x04, 0x04, 0x04, 0x04, 0x04 }, // 1
  { 0x07, 0x04, 0x07, 0x01, 0x07 }, // 2
  { 0x07, 0x04, 0x07, 0x04, 0x07 }, // 3
  { 0x05, 0x05, 0x07, 0x04, 0x04 }, // 4
  { 0x07, 0x01, 0x07, 0x04, 0x07 }, // 5
  { 0x07, 0x01, 0x07, 0x05, 0x07 }, // 6
  { 0x07, 0x04, 0x04, 0x04, 0x04 }, // 7
  { 0x07, 0x05, 0x07, 0x05, 0x07 }, // 8
  { 0x07, 0x05, 0x07, 0x04, 0x07 }  // 9
};
void ada88_writeNumber( int num )	//	num 1999 .. -1999
{
	int i;
	unsigned char i2cBufx[17];
	unsigned char ledPtn[8] = {0};

	static const unsigned char graph[10][2] PROGMEM = {
		{ 0x00, 0x00 },
		{ 0x00, 0x40 },
		{ 0x40, 0x60 },
		{ 0x60, 0x70 },
		{ 0x70, 0x78 },
		{ 0x78, 0x7c },
		{ 0x7c, 0x7e },
		{ 0x7e, 0x7f },
		{ 0x7f, 0xff },
		{ 0xff, 0xff }
	};

	if ( num > 1999 ){ num = 1999; }
	else if ( num < -1999 ){ num = -1999;}

	//	+/-, over 1000 or not
	if ( num/1000 ){ ledPtn[5] |= 0x80; }
	if ( num<0 ){
		ledPtn[2] |= 0x80;
		num = -num;
	}

	int num3digits = num%1000;
	int hundred = num3digits/100;
	int num2degits = num3digits%100;
	int deci = num2degits/10;
	int z2n = num2degits%10;

	for ( i=0; i<5; i++ ){
		ledPtn[i] |= pgm_read_byte(&(numletter[hundred][i]));
	}
	for ( i=0; i<5; i++ ){
		ledPtn[i] |= (pgm_read_byte(&(numletter[deci][i]))<<4);
	}
	for ( i=0; i<2; i++ ){
		ledPtn[i+6] = pgm_read_byte(&(graph[z2n][i]));
	}

	i2cBufx[0] = 0;
	for ( i=0; i<8; i++ ){
		i2cBufx[i*2+1] = ledPtn[i];
		i2cBufx[i*2+2] = 0;
	}
	write_i2cDevice( ADA88_I2C_ADRS, i2cBufx, 17 );
}
//---------------------------------------------------------
void ada88_write_5param(uint8_t prm1, uint8_t prm2, uint8_t prm3, uint8_t prm4, uint8_t prm5)
//  prm1:0-9, prm2:0-7, prm3:0-7, prm4:0-7, prm0-7
{
  int i;
  unsigned char i2cBufx[17];
  unsigned char ledPtn[8] = {0};

  if (prm1>9){prm1=9;}
  if (prm2>7){prm2=7;}
  if (prm3>7){prm3=7;}
  if (prm4>7){prm4=7;}
  if (prm5>7){prm5=7;}

  for (i=0; i<5; i++){
    ledPtn[i+1] |= (pgm_read_byte(&(numletter[prm1][i]))<<1);
  }

  for (i=0; i<prm2; i++){ //  horizontal
    ledPtn[7] |= (0x01<<i);
  }
  ledPtn[7] |= 0x80;      //  always ON
  for (i=0; i<prm3; i++){ //  vertical L
    ledPtn[6-i] |= 0x80; 
  }

  for (i=0; i<prm4; i++){ //  vertical R1
    ledPtn[6-i] |= 0x20;
  }
  for (i=0; i<prm5; i++){ //  vertical R2
    ledPtn[6-i] |= 0x40;
  }

  i2cBufx[0] = 0;
  for ( i=0; i<8; i++ ){
    i2cBufx[i*2+1] = ledPtn[i];
    i2cBufx[i*2+2] = 0;
  }
  write_i2cDevice( ADA88_I2C_ADRS, i2cBufx, 17 );
}
void ada88_write_bit(uint16_t num) //	num 0x0000 - 0xffff
{
    //  oooooooo : No Light
    //  oooooooo : No Light
    //  oooooooo : No Light
    //  oooooooo : No Light
    //  oooooooo : No Light
    //  oooooooo : No Light
    //  01234567 : Upper 8bit Display
    //  01234567 : Lower 8bit Display
    //------------
    uint8_t i2c_bufx[17] = {0};
    uint8_t led_ptn[8] = {0};

    //  upper 8bit
    uint8_t lsb = 0x00;
    uint8_t bits = ((num >> 8) & 0x00ff);
    if ((bits & 0x01) != 0) {
        lsb = 0x80;
    }
    led_ptn[6] = (bits >> 1) + lsb; // upper

    //  lower 8bit
    bits = static_cast<uint8_t>(num & 0x00ff);
    lsb = 0x00;
    if ((bits & 0x01) != 0) {
        lsb = 0x80;
    }
    led_ptn[7] = (bits >> 1) + lsb; // lower

    //  Set LED I2C Buffer
    i2c_bufx[0] = 0;
    for (int i=0; i<8; ++i) {
        i2c_bufx[i * 2 + 1] = led_ptn[i];
        i2c_bufx[i * 2 + 2] = 0;
    }
    write_i2cDevice(ADA88_I2C_ADRS, i2c_bufx, 17);
}
//---------------------------------------------------------
void ada88_write_org_bit(bool* bit) {
    uint16_t num = 0; //	num 0x0000 - 0xffff
    for (int i=0; i<16; i++) {
        if (bit[i]) {
            num += 0x0001 << i;
        }
    }
    ada88_write_bit(num);
}
//---------------------------------------------------------
void ada88_anime( int time )
{
	static const unsigned char bitmap[64] PROGMEM = {
    // "Touch Above    "
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x40,0x40,0x7e,0x40,0x5e,0x12,0x1e,0x00,0x1e,0x02,0x1e,0x02,0x00,0x1e,0x12,
    0x1a,0x00,0x7e,0x08,0x0e,0x00,0x00,0x00,0x7e,0x48,0x7e,0x00,0x7e,0x0a,0x0e,0x00,
    0x1e,0x12,0x1e,0x00,0x1c,0x02,0x1c,0x00,0x3e,0x2a,0x3a,0x00,
  };
  // Data 読み込み
	unsigned char tmpBuf[8];
  int SFT[8] = {0,7,6,5,4,3,2,1};
  for (int i=0; i<8; i++) {
    tmpBuf[SFT[i]] = pgm_read_byte(&(bitmap[((time+i)%64)]));
  }
  // bit単位で書き込みデータを作成
	unsigned char i2cBufx[17] = {0};
	for (int i=0; i<8; i++){
    char bit = 0x80 >> i;
    unsigned char reg = 0;
    for (int j=0; j<8; j++){
      if ((tmpBuf[j] & bit) != 0) {
        reg |= (0x80 >> j);
      };
    }
		i2cBufx[i*2+1] = reg;
		i2cBufx[i*2+2] = 0;
	}
	write_i2cDevice( ADA88_I2C_ADRS, i2cBufx, 17 );
}
#endif


//-------------------------------------------------------------------------
//			PCA9685 (LED Driver : I2c Device)
//-------------------------------------------------------------------------
#ifdef USE_PCA9685    //	for LED Driver
constexpr unsigned char PCA9685_ADDRESS = 0x40;
//-------------------------------------------------------------------------
int PCA9685_write( uint8_t chipNumber, uint8_t cmd1, uint8_t cmd2 )
{
	unsigned char	i2cBuf[2];
	i2cBuf[0] = cmd1; i2cBuf[1] = cmd2;
	int err = write_i2cDevice( PCA9685_ADDRESS+chipNumber, i2cBuf, 2 );
	return err;
}
//-------------------------------------------------------------------------
//		Initialize
//-------------------------------------------------------------------------
void PCA9685_init( uint8_t chipNumber )
{
	//	Init Parameter
	PCA9685_write( chipNumber, 0x00, 0x00 );
	PCA9685_write( chipNumber, 0x01, 0x12 );//	Invert, OE=high-impedance
}
//-------------------------------------------------------------------------
//		rNum, gNum, bNum : 0 - 4094  bigger, brighter
//-------------------------------------------------------------------------
int PCA9685_setFullColorLED( uint8_t chipNumber, int ledNum, unsigned short* color  )
{
  int err = 0;
	int	i;

  ledNum &= 0x03;
	for ( i=0; i<3; i++ ){
		//	figure out PWM counter
		unsigned short colorCnt = *(color+i);
		colorCnt = 4095 - colorCnt;
		if ( colorCnt <= 0 ){ colorCnt = 1;}

		//	Set PWM On Timing
		err = PCA9685_write( chipNumber, (uint8_t)(0x06 + i*4 + ledNum*16), (uint8_t)(colorCnt & 0x00ff) );
        if ( err != 0 ){ return err; }
		err = PCA9685_write( chipNumber, (uint8_t)(0x07 + i*4 + ledNum*16), (uint8_t)((colorCnt & 0xff00)>>8) );
        if ( err != 0 ){ return err; }

		err = PCA9685_write( chipNumber, (uint8_t)(0x08 + i*4 + ledNum*16), 0 );
        if ( err != 0 ){ return err; }
		err = PCA9685_write( chipNumber, (uint8_t)(0x09 + i*4 + ledNum*16), 0 );
        if ( err != 0 ){ return err; }
	}
  return err;
}
#endif


#ifdef USE_PCA9544A
//---------------------------------------------------------
//		<< PCA9544A >>
//---------------------------------------------------------
static const unsigned char PCA9544A_I2C_ADRS = 0x70;
//-------------------------------------------------------------------------
//			PCA9544A ( I2C Multiplexer : I2c Device)
//-------------------------------------------------------------------------
int pca9544_changeI2cBus(int i2c_num, int dev_num)
{
	uint8_t	i2cBuf = 0x04 | static_cast<uint8_t>(i2c_num&0x0003);
  uint8_t i2cadrs = PCA9544A_I2C_ADRS + static_cast<uint8_t>(dev_num);
	int		err = 0;
	err = write_i2cDevice(i2cadrs, &i2cBuf, 1);
	return err;
}
#endif

//-------------------------------------------------------------------------
//			SSD1331 (OLED Driver : SPI Device)
//-------------------------------------------------------------------------
#ifdef USE_SSD1331
#include <SPI.h>
#include <Adafruit_SSD1331.h>
#include <Adafruit_GFX.h>

//displayピン設定
#define DISPLAY_CS    29
#define DISPLAY_RST    0
#define DISPLAY_DC    28
#define DISPLAY_MOSI   3
#define DISPLAY_SCK    2

// JPGの最大サイズ(バッファを静的に確保するようにしているため、決め打ち。取り扱う最大ファイルサイズで変えるようにする)
#define JPG_SIZE_MAX (20 * 1024) //MAX 20KByteを想定

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, DISPLAY_CS, DISPLAY_DC, DISPLAY_RST);

void SSD1331_init(void) {
    //SPIピン設定
    SPI.setTX(DISPLAY_MOSI);
    SPI.setSCK(DISPLAY_SCK);
  
    //displayの初期化と初期設定
    display.begin();       
    display.fillScreen(BLACK);                      //背景の塗りつぶし
}
void SSD1331_display(const char* str, int line) {
  display.fillRect(0, 10*line, 100, 10, BLACK);
  // 0° (デフォルト) の向きで描画
  display.setRotation(0);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10*line);
  display.print(str);
} 
#endif
/* [] END OF FILE */
