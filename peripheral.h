/* ========================================
 *
 *	i2cdevice.h
 *		description: TouchMidi I2C Device Driver
 *
 *	Copyright(c)2018- Masahiko Hasebe at Kigakudoh
 *  This software is released under the MIT License, see LICENSE.txt
 *
 * ========================================
*/
#ifndef I2CDEVICE_H
#define I2CDEVICE_H

void wireBegin( void );
void initHardware( void );

// AT42QT
void AT42QT_init( void );
int AT42QT_read( size_t key, uint8_t (&rdraw)[2], bool ref );

// USE_ADA88
	void ada88_init( void );
	void ada88_write( int letter );
  void ada88_writeBit( uint16_t num );
	void ada88_writeNumber( int num );
  void ada88_write_5param(uint8_t prm1, uint8_t prm2, uint8_t prm3, uint8_t prm4, uint8_t prm5);
  void ada88_write_bit(uint16_t num);
  void ada88_write_org_bit(bool* bit);
  void ada88_anime( int time );

// USE_PCA9544A
  int pca9544_changeI2cBus(int i2c_num, int dev_num);

// USE_PCA9685
	void PCA9685_init( uint8_t chipNumber );
  int PCA9685_write( uint8_t chipNumber, uint8_t cmd1, uint8_t cmd2 );
	int PCA9685_setFullColorLED( uint8_t chipNumber, int ledNum, unsigned short* color  );

// USE_SSD1331
  // Color definitions
  enum class SSD1331_COLORS {
      BLACK      = 0x0000,
      BLUE       = 0x001F,
      RED        = 0xF800,
      GREEN      = 0x07E0,
      CYAN       = 0x07FF,
      MAGENTA    = 0xF81F,
      YELLOW     = 0xFFE0,
      WHITE      = 0xFFFF
  };
  void SSD1331_init(void);
  void SSD1331_clear(void);
  void SSD1331_display(const char* str, int line, SSD1331_COLORS color);

#endif
