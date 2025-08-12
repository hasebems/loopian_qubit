//  Created by Hasebe Masahiko on 2025/04/21.
//  Copyright (c) 2025 Hasebe Masahiko.
//  Released under the MIT license
//  https://opensource.org/licenses/mit-license.php
//
#include <sstream>   // ostringstream
#include <iomanip> 
#include <format>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
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

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// Create a neopixel object
SK6812 sk(MAX_LIGHT, 26);

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
QubitTouch qt([](uint8_t status, uint8_t note, uint8_t intensity) {
  // MIDI callback function
  sendMidiMessage(status, note, intensity);
});

/*----------------------------------------------------------------------------*/
//     setup
/*----------------------------------------------------------------------------*/
void setup() {
  Serial.begin(115200);

  // Device Discriptor
  TinyUSBDevice.setManufacturerDescriptor("Kigakudoh");
  TinyUSBDevice.setProductDescriptor("Loopian::QUBIT");
  // MIDI Port Name
  usb_midi.setStringDescriptor("Loopian::QUBIT MIDI");

  // initialize USB MIDI
  usb_midi.begin();
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // MIDI Callbacks
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleProgramChange(handleProgramChange);

  // wait until device mounted : 挿さないと起動しなくなるので削除
  //while( !TinyUSBDevice.mounted() ) delay(1);

  // GPIO  
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  gpio_put(LED_BLUE, HIGH);
  gpio_put(LED_GREEN, HIGH);
  gpio_put(LED_RED, HIGH);

  SSD1331_init();

  // I2C
  wireBegin();
  AT42QT_init();

  // Interval in unsigned long microseconds
  if (!ITimer1.attachInterruptInterval(2000, TimerHandler)) { // 2ms
    gpio_put(LED_RED, LOW);
    assert(false); // Timer initialization failed
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

  Serial.println("Loopian::QUBIT started");
}
/*----------------------------------------------------------------------------*/
//     loop
/*----------------------------------------------------------------------------*/
void loop() {
  check_usb_status();

  //  Global Timer 
  long difftm = generateTimer();
  int cnt = gt.timer100ms()%10;
  bool stable = gt.timer1s() > 5;
  // Heartbeat LED
  if (cnt<5){
    gpio_put(LED_BLUE, LOW);
  }
  else {
    gpio_put(LED_BLUE, HIGH);
  }

  // read any new MIDI messages
  MIDI.read();
  if (gt.timer10msecEvent()) {
    int sv[MAX_SENS] = {0};
    for (int i = 0; i < MAX_SENS; i++) {
      int sensor_value = get_sensor_values(i);
      qt.set_value(i, sensor_value);
      sv[i] = sensor_value;
    }
    show_one_line(0, sv[0], sv[1]);
    show_one_line(1, sv[2], sv[3]);
    show_one_line(2, sv[4], sv[5]);
    if (stable) {
      qt.seek_and_update_touch_point();
      clear_touch_leds();
      qt.lighten_leds(callback_for_set_led);
    }
  }

  // Lighten LEDs (NeoPixel)
  set_led_for_wave(gt.globalTime());
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
void check_usb_status() {
  static bool usb_connected = false;
  if (usb_connected) {return;}
  if (TinyUSBDevice.mounted()) {
    usb_connected = true;
    Serial.println("USB connected");
  }
}
/*----------------------------------------------------------------------------*/
//     Read AT42QT1070 raw/ref values
//      num: 0-15 (which AT42QT1070 to read from)
//      sens: 0-5 (which sensor to read from, 0-5)
//      ref: true for reference value, false for raw value
//      returns: tuple of error code and raw value
/*----------------------------------------------------------------------------*/
std::tuple<int, uint16_t> read_from_AT42QT(int num, int sens, bool ref) {
  uint8_t raw[2];
  constexpr uint8_t CONVERT_TO_DEV_NUM[4] = {2, 3, 1, 0};// {3,2,1,0}
  constexpr uint8_t OFFSET_I2C_ADRS[4] = {0x04, 0x05, 0x06, 0x07}; // {0,1,2,3}
  pca9544_changeI2cBus(CONVERT_TO_DEV_NUM[num % 4], OFFSET_I2C_ADRS[num / 4]);
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
          Serial.println("USB MIDI: Note On");
          Serial.println(note);
          MIDI.sendNoteOn(note, velocity, channel);
          break;
      case 0x80: // Note Off
          Serial.println("USB MIDI: Note Off");
          Serial.println(note);
          MIDI.sendNoteOff(note, velocity, channel);
          break;
      case 0xB0: // Control Change
          //MIDI.sendControlChange(note, velocity, channel);
          break;
      case 0xC0: // Program Change
          //MIDI.sendProgramChange(note, channel);
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
    //std::string temp_str = std::to_string(gt.timer1s());
    std::string temp_str = std::to_string(qt.get_touch_count());
    //temp_str += "/" + std::to_string(MAX_TOUCH_POINTS);
    //temp_str += ":" + std::to_string(MAX_SENS);
    //const char* dbg_info = temp_str.c_str();

    float disp_loc = qt.touch_point(0).get_location();
    if (disp_loc == TouchPoint::INIT_VAL) {
      temp_str += " L:---";
    } else {
      auto loc = std::ostringstream();
      loc << std::fixed << std::setprecision(1) << disp_loc;
      temp_str += " L:" + loc.str();
    }
    auto loc2 = std::ostringstream();
    loc2 << std::fixed << std::setprecision(1) << qt.touch_point(0).get_intensity();
    temp_str += "/" + loc2.str();
    //auto loc3 = std::ostringstream();
    //loc3 << std::fixed << std::setprecision(1) << qt.deb_val();
    //temp_str += " D:" + loc3.str();
    SSD1331_display(temp_str.c_str(), 5);
}
/*----------------------------------------------------------------------------*/
//     NeoPixel
/*----------------------------------------------------------------------------*/
uint8_t neo_pixel[MAX_LIGHT][4];
void init_neo_pixel() {
  // Set up the sk6812
  sk.begin();
  for (int i = 0; i < MAX_LIGHT; i++) {
    neo_pixel[i][0] = 0; // red
    neo_pixel[i][1] = 0; // green
    neo_pixel[i][2] = 0; // blue
    neo_pixel[i][3] = 0; // white
  }
  update_neo_pixel();
}
void clear_touch_leds() {
    for (int i = 0; i < MAX_LIGHT; i++) {
      neo_pixel[i][0] = 0; // red
      neo_pixel[i][1] = 0; // green
      neo_pixel[i][2] = 0; // blue
  }
}
//-----------------------------------------------------------
void callback_for_set_led(float locate, int16_t sensor_value) {
  if ((locate < 0.0f) || (locate >= static_cast<float>(MAX_SENS))){
    return; // Invalid location
  }
  if (sensor_value <= 1) {
    sensor_value = 1;
  }

  const float SLOPE = 20000.0f / sensor_value; // 傾き:小さいほどたくさん光る
  float nearest_lower = std::floor(locate);
  float nearest_upper = std::ceil(locate);

  while (1) {
    int16_t this_val = static_cast<int16_t>(255 - (locate - nearest_lower)*SLOPE);
    if (this_val < 0) {break;}
    set_led_for_touch(static_cast<int>(nearest_lower), this_val);
    nearest_lower -= 1.0f;
  }
  while (1) {
    int16_t this_val = static_cast<int16_t>(255 - (nearest_upper - locate)*SLOPE);
    if (this_val < 0) {break;}
    set_led_for_touch(static_cast<int>(nearest_upper), this_val);
    nearest_upper += 1.0f;
  }
}
//-----------------------------------------------------------
void set_led_for_touch(int index, uint8_t intensity) {
  uint8_t red = (intensity * 4) / 5;
  uint8_t blue = (intensity * 1) / 5;
  set_neo_pixel(index, red, 0, blue, -1); // Set only red and blue channels
}
//-----------------------------------------------------------
void set_led_for_wave(uint16_t global_time) {
  float tm = static_cast<float>(global_time); // convert to seconds
  for (int i = 0; i < MAX_LIGHT; i++) {
    float phase = (tm * 0.002f + static_cast<float>(i) * 0.1f) * 2 * PI;
    uint8_t intensity = static_cast<uint8_t>(10.0f * (std::sin(phase)) + 10.0f);
    set_white_led(i, intensity);
  }
}
//-----------------------------------------------------------
void set_white_led(int index, uint8_t intensity) {
  set_neo_pixel(index, -1, -1, -1, intensity); // Set only red and blue channels
}
//-----------------------------------------------------------
void set_neo_pixel(int index, int16_t red, int16_t green, int16_t blue, int16_t white) {
  while (index < 0) {
    index += MAX_LIGHT; // Wrap around if negative
  }
  index %= MAX_LIGHT;
  if (red != -1)    {neo_pixel[index][0] = static_cast<uint8_t>(red);}
  if (green != -1)  {neo_pixel[index][1] = static_cast<uint8_t>(green);}
  if (blue != -1)   {neo_pixel[index][2] = static_cast<uint8_t>(blue);}
  if (white != -1)  {neo_pixel[index][3] = static_cast<uint8_t>(white);}
}
void update_neo_pixel() {
  sk.clear();
  for (int i = 0; i < MAX_LIGHT; i++) {
    sk.setPixelColor(i, neo_pixel[i][0], neo_pixel[i][1], neo_pixel[i][2], neo_pixel[i][3]);
  }
  sk.show();
}
