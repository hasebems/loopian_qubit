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

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  RP2040 を使う時は Wire. で、RP2350 を使う時は Wire1. にする
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//---------------------------------------------------------
//		Initialize I2C Device
//---------------------------------------------------------
void wireBegin( void )
{
  Wire1.setClock(400000);
  Wire1.setSDA(6);
  Wire1.setSCL(7);
	Wire1.begin();
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
	Wire1.beginTransmission(adrs);
  Wire1.write(buf,count);
	return Wire1.endTransmission();
}
//---------------------------------------------------------
//		Read 1byte I2C Device
//---------------------------------------------------------
int read1byte_i2cDevice( unsigned char adrs, unsigned char* wrBuf, unsigned char* rdBuf, int wrCount )
{
	unsigned char err;

	Wire1.beginTransmission(adrs);
  Wire1.write(wrBuf,wrCount);
	err = Wire1.endTransmission(false);
	if ( err != 0 ){ return err; }

	err = Wire1.requestFrom(adrs,(uint8_t)1,(uint8_t)0);
	while(Wire1.available()) {
		*rdBuf = Wire1.read();
	}

	//err = Wire1.endTransmission(true);
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

	Wire1.beginTransmission(adrs);
  Wire1.write(wrBuf,wrCount);
	err = Wire1.endTransmission(false);
	if ( err != 0 ){ return err; }

	err = Wire1.requestFrom(adrs,static_cast<uint8_t>(rdCount),(uint8_t)0);
	int rdAv = 0;
	while((rdAv = Wire1.available()) != 0) {
		*(rdBuf+rdCount-rdAv) = Wire1.read();
	}

	//err = Wire1.endTransmission(true);
	//return err;

	return 0;
}
// ハングしてはいけない本番用
int read_nbyte_i2cDeviceX( unsigned char adrs, unsigned char* wrBuf, unsigned char* rdBuf, int wrCount, int rdCount )
{
	unsigned char err;
  volatile int cnt=0;

	Wire1.beginTransmission(adrs);
  Wire1.write(wrBuf,wrCount);
	err = Wire1.endTransmission(false);
	if ( err != 0 ){ return err; }

	err = Wire1.requestFrom(adrs,static_cast<uint8_t>(rdCount),(uint8_t)0);
	int rdAv = 0;
	while(((rdAv = Wire1.available()) != 0) && (cnt < 10)){
		*(rdBuf+rdCount-rdAv) = Wire1.read();
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

  err = Wire1.requestFrom(adrs,static_cast<uint8_t>(rdCount),static_cast<uint8_t>(false));
  int rdAv = Wire1.available();
  while( rdAv ) {
    *(rdBuf+rdCount-rdAv) = Wire1.read();
    rdAv--;
  }

  err = Wire1.endTransmission(true);
  return err;
}

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
static uint8_t old_dev_num = 0;
//-------------------------------------------------------------------------
//			PCA9544A ( I2C Multiplexer : I2c Device)
//        i2c_num:  0..3 (which I2C bus to use)
//        dev_num:  0..7 (which device to select)
//-------------------------------------------------------------------------
int pca9544_changeI2cBus(int i2c_num, int dev_num)
{
  uint8_t sub_i2c_num = i2c_num & 0x0003;
  if (dev_num != old_dev_num) { // 前回と違うデバイスなら、前のデバイスは接続を切る
    uint8_t stop_cnct = 0x00;
    uint8_t old_adrs = PCA9544A_I2C_ADRS + static_cast<uint8_t>(old_dev_num);
    int err0 = write_i2cDevice(old_adrs, &stop_cnct, 1);
    old_dev_num = dev_num;
  }
	uint8_t	i2cBuf = 0x04 | static_cast<uint8_t>(sub_i2c_num);
  uint8_t i2cadrs = PCA9544A_I2C_ADRS + static_cast<uint8_t>(dev_num);
  int err = write_i2cDevice(i2cadrs, &i2cBuf, 1);
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

// Seeed XIAO displayピン設定
#define DISPLAY_CS     D3
#define DISPLAY_RST    D6
#define DISPLAY_DC     D2
#define DISPLAY_MOSI   D10
#define DISPLAY_SCK    D8

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
