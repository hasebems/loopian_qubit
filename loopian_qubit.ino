//  Created by Hasebe Masahiko on 2025/04/21.
//  Copyright (c) 2025 Hasebe Masahiko.
//  Released under the MIT license
//  https://opensource.org/licenses/mit-license.php
//
#include <string>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include <RPi_Pico_TimerInterrupt.h> //https://github.com/khoih-prog/RPI_PICO_TimerInterrupt

#include  "sk6812.h"
#include  "peripheral.h"
#include  "global_timer.h"
#include  "qtouch.h"
#include  "constants.h"

/*----------------------------------------------------------------------------*/
//     Constants
/*----------------------------------------------------------------------------*/
#define LED_BLUE      25    // LED
#define LED_GREEN     16    // LED
#define LED_RED       17    // LED
#define POWER_PIN     23    //NeoPixelの電源

/*----------------------------------------------------------------------------*/
//     Variables
/*----------------------------------------------------------------------------*/
// USB MIDI object
Adafruit_USBD_MIDI usb_midi;
//Adafruit_NeoPixel pixels(1, 22);

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// Create a neopixel object
SK6812 sk(MAX_SENS, 26);

// Init RPI_PICO_Timer
RPI_PICO_Timer ITimer1(1);

GlobalTimer gt;

struct OneTouch {
  int raw_value;
  int ref_value;
};
OneTouch tch[MAX_SENS];
char text_display[7][16];
int sensor_adjust_counter;
QubitTouch qt([](uint8_t pad_num, uint8_t value, uint8_t intensity) {
  // MIDI callback function
  sendMidiMessage((value > 0) ? 0x90 : 0x80, pad_num, intensity);
});

/*----------------------------------------------------------------------------*/
//     setup
/*----------------------------------------------------------------------------*/
void setup() {
  // GPIO  
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  gpio_put(LED_BLUE, HIGH);
  gpio_put(LED_GREEN, HIGH);
  gpio_put(LED_RED, HIGH);

  SSD1331_init();

  //Neopixelの電源供給開始
  //pinMode(POWER_PIN, OUTPUT);
  //gpio_put(POWER_PIN, HIGH);
  //pixels.begin();             //NeoPixel制御開始
  //pixels.setBrightness(128);  //NeoPixelの明るさ設定
  //pixels.setPixelColor(0, pixels.Color(255, 255, 255));
  //pixels.show();
  // 消灯
  //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  //pixels.show();

  //TinyUSB_Device_Init(0);
  //MIDI.setHandleNoteOn(handleNoteOn);
  //MIDI.setHandleNoteOff(handleNoteOff);
  //MIDI.setHandleProgramChange(handleProgramChange);
  //usb_midi.setStringDescriptor("TinyUSB MIDI");
  //MIDI.begin(MIDI_CHANNEL_OMNI);
  //MIDI.turnThruOff();

  // wait until device mounted
  //while( !TinyUSBDevice.mounted() ) delay(1);

  // I2C
  wireBegin();
  AT42QT_init();

  // Interval in unsigned long microseconds
  if (ITimer1.attachInterruptInterval(2000, TimerHandler)) { // 2ms
    // success
  }

  text_display[7][16] = {0};
  sensor_adjust_counter = 0;
  init_neo_pixel();

  for (int i = 0; i < MAX_SENS; i++) {
    uint8_t raw[2];
    int err = AT42QT_read(i, raw, true);
    int refval = static_cast<int>(raw[0]) * 256 + raw[1];
    tch[i].raw_value = 0;
    tch[i].ref_value = refval;
  }
}
/*----------------------------------------------------------------------------*/
//     loop
/*----------------------------------------------------------------------------*/
void loop() {
  //  Global Timer 
  long difftm = generateTimer();
  int cnt = gt.timer100ms()%10;
  if (cnt<5){
    gpio_put(LED_BLUE, LOW);
  }
  else {
    gpio_put(LED_BLUE, HIGH);
  }

  // read any new MIDI messages
  //MIDI.read();
  if (gt.timer10msecEvent()) {
    int sv[MAX_SENS] = {0};
    for (int i = 0; i < MAX_SENS; i++) {
      int sensor_value = get_sensor_values(i);
      qt.set_value(i, sensor_value);
      set_color(i, sensor_value*5, 0, sensor_value*2, sensor_value*5);
      sv[i] = sensor_value;
    }
    show_one_line(0, sv[0], sv[1]);
    show_one_line(1, sv[2], sv[3]);
    show_one_line(2, sv[4], sv[5]);
    qt.seek_and_update_touch_point();
  }

  // Lighten LEDs (NeoPixel)
  update_neo_pixel();

  if (gt.timer100msecEvent()) {
    show_debug_info();

    // Update sensor ref values
    read_ref_sensor_values(sensor_adjust_counter);
    sensor_adjust_counter++;
    if (sensor_adjust_counter >= MAX_SENS) {
      sensor_adjust_counter = 0;
    }
  }
}
/*----------------------------------------------------------------------------*/
//     Read AT42QT1070 raw/ref values
/*----------------------------------------------------------------------------*/
std::tuple<int, uint16_t> read_from_AT42QT(int num, int sens, bool ref) {
  static int old_num = -1;
  uint8_t raw[2];
  if (num != old_num) {
    int real_num = 15 - num;
    pca9544_changeI2cBus(real_num%4, real_num/4);
    old_num = num;
  }
  int err = AT42QT_read(sens, raw, ref);
  uint16_t rawval = static_cast<uint16_t>(raw[0]) * 256 + raw[1];
  return {err, rawval};
}
int get_sensor_values(int sens) {
  std::tuple<int, uint16_t> result = read_from_AT42QT(sens/6, sens%6, false);
  int rawval = std::get<1>(result);
  int diff = rawval - tch[sens].ref_value;
  if (diff < 0) {diff = 0;}
  tch[sens].raw_value = rawval;
  return diff;
}
void read_ref_sensor_values(int sens) {
  std::tuple<int, uint16_t> result = read_from_AT42QT(sens/6, sens%6, true);
  if (tch[sens].ref_value != std::get<1>(result)) {
    tch[sens].ref_value = std::get<1>(result);
  }
}
/*----------------------------------------------------------------------------*/
//     Timer
/*----------------------------------------------------------------------------*/
bool TimerHandler(repeating_timer* rt)
{
  gt.incGlobalTime();
  return true;
}
long generateTimer( void )
{
  uint16_t  gTime = gt.globalTime();
  long diff = gTime - gt.gtOld();
  gt.setGtOld(gTime);
  if ( diff < 0 ){ diff += 0x10000; }

  gt.clearAllTimerEvent();
  gt.updateTimer(diff);
  return diff;
}
/*----------------------------------------------------------------------------*/
//     MIDI/Other Hardware
/*----------------------------------------------------------------------------*/
void handleNoteOn(byte channel, byte pitch, byte velocity) {
}
void handleNoteOff(byte channel, byte pitch, byte velocity) {
}
void handleProgramChange(byte channel , byte number) {
}
void sendMidiMessage(uint8_t status, uint8_t note, uint8_t velocity) {
  uint8_t channel = (status & 0x0F) + 1;
  switch(status & 0xF0) {
      case 0x90: // Note On
          MIDI.sendNoteOn(note, velocity, channel);
          break;
      case 0x80: // Note Off
          MIDI.sendNoteOff(note, velocity, channel);
          break;
      case 0xB0: // Control Change
          MIDI.sendControlChange(note, velocity, channel);
          break;
      case 0xC0: // Program Change
          MIDI.sendProgramChange(note, channel);
          break;
      default:
          break;
  }
}
/*----------------------------------------------------------------------------*/
//     Display for SSD1331
/*----------------------------------------------------------------------------*/
void show_one_line(int line, int value1, int value2) {
    if (line < 0 || line >= 3) {
        return; // Invalid line number
    }
    std::fill(std::begin(text_display[line]), std::end(text_display[line]), 32);
    size_t idx = 0;
    for (int i=0; i<8; i++) {
      if (value1 > 4) {
        text_display[line][idx] = 'o';
        idx += 1;
        value1 -= 4;
      } else {
        break;
      }
    }
    idx = 8;
    for (int j=0; j<8; j++) {
      if (value2 > 4) {
        text_display[line][idx] = 'x';
        idx += 1;
        value2 -= 4;
      } else {
        break;
      }
    }
    SSD1331_display(text_display[line], line);
}
void show_debug_info() {
    std::string temp_str = std::to_string(gt.timer1s()) ;
    temp_str += " tch:" + std::to_string(qt.get_touch_count());
    temp_str += "/" + std::to_string(MAX_TOUCH_POINTS);
    temp_str += ":" + std::to_string(MAX_SENS);
    const char* dbg_info = temp_str.c_str();
    SSD1331_display(dbg_info, 5);  
}
/*----------------------------------------------------------------------------*/
//     NeoPixel
/*----------------------------------------------------------------------------*/
uint8_t neo_pixel[MAX_SENS][4];
void init_neo_pixel() {
  // Set up the sk6812
  sk.begin();
  for (int i = 0; i < MAX_SENS; i++) {
    neo_pixel[i][0] = 0; // red
    neo_pixel[i][1] = 0; // green
    neo_pixel[i][2] = 0; // blue
    neo_pixel[i][3] = 0; // white
  }
  update_neo_pixel();
}
void set_color(int locate, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
  if (locate < 0 || locate >= MAX_SENS) {
    return; // Invalid location
  }
  neo_pixel[locate][0] = red;
  neo_pixel[locate][1] = green;
  neo_pixel[locate][2] = blue;
  neo_pixel[locate][3] = white;
}
void update_neo_pixel() {
  sk.clear();
  for (int i = 0; i < MAX_SENS; i++) {
    sk.setPixelColor(i, neo_pixel[i][0], neo_pixel[i][1], neo_pixel[i][2], neo_pixel[i][3]);
  }
  sk.show();
}
