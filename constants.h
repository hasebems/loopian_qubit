/* ========================================
 *
 *	constants.h
 *		description: TouchMidi Configuration
 *
 *	Copyright(c)2017- Masahiko Hasebe at Kigakudoh
 *  This software is released under the MIT License, see LICENSE.txt.
 *
 * ========================================
*/
#ifndef CONSTANTS_H
#define CONSTANTS_H

constexpr int CUBIT_VERSION = 0;

//#define TEST_MODE
#ifdef TEST_MODE
constexpr int MAX_KAMABOKO_NUM = 1;
#else
constexpr int MAX_KAMABOKO_NUM = 4;
#endif

constexpr int MAX_EACH_SENS = 6;
constexpr int MAX_SENS = MAX_EACH_SENS*MAX_KAMABOKO_NUM; // 全センサの数

constexpr size_t MAX_NOTE = 96; // 1system が取りうる最大 Note 番号
constexpr int MAX_LOCATE = MAX_NOTE*100;
constexpr int MAX_EACH_LIGHT = 6;
constexpr int MAX_LIGHT = MAX_EACH_LIGHT*MAX_KAMABOKO_NUM;

constexpr size_t MAX_TOUCH_POINTS = 4; // Maximum number of touch points to track

constexpr int HOLD_TIME = 10;  // *10msec この間、一度でもonならonとする。離す時少し鈍感にする。最大16
constexpr long UPDATE_TIME = 25; // White LED の背景放射の更新時間 *2[msec]

constexpr long JSTICK_LONG_HOLD_TIME = 1000; // 曲選択モードにするために JoyStick を長押しする時間 x/500[sec]

//  MIDI Note Number
constexpr int KEYBD_LO = 21; // A0
constexpr int KEYBD_C1 = 24;
constexpr int KEYBD_HI = 108; // C8
constexpr int MAX_MIDI_NOTE = 128;

//  Hardware
constexpr uint8_t PCA9685_OFSADRS = 16;

//#define USE_CY8CMBR3110
//#define USE_ADA88
//#define USE_PCA9685
#define USE_AT42QT1070
#define USE_PCA9544A
#define USE_SSD1331

void sendMidiMessage(uint8_t status, uint8_t note, uint8_t velocity);

#endif
