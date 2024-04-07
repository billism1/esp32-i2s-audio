#include <Arduino.h>
#include "AudioTools.h"
// #include "AudioEffects/AudioEffect.h"
//  #include <SPIFFS.h>

#define I2S_MIC_SD 14
#define I2S_MIC_WS 13
#define I2S_MIC_SCK 12

// MAX98357A pins:
// LRC:  I2S WS
// BCLK: I2S SCK
// DIN:  I2S SD
// GAIN: Use resistor to set
// SD:   L-R Output select
// GND:  Ground
// Vin:  3 - 5.5 VDC

// pin reference: https://dronebotworkshop.com/esp32-i2s/

#define I2S_SPEAKER_BCLK 7
#define I2S_SPEAKER_LRC 15
#define I2S_SPEAKER_LCK 15 // LCK same as LRC
#define I2S_SPEAKER_DIN 16

#define VOLUME_PIN 4
#define PITCH_PIN 2

// float pitchShift = 1.3;
// float volume = 0.7;
float pitchValue = 1.0;
float volumeValue = 1.0;
// uint16_t sample_rate=44100;
int sampleRate = 16000;
int channels = 1;
int bitsPerChannel = 16;

AudioInfo audioInfo(sampleRate, channels, bitsPerChannel);
I2SStream inStream;
I2SStream outStream;
VolumeStream volumeStream(inStream);

AudioEffectStream effects(outStream);
PitchShift pitchShiftEffect(pitchValue);

StreamCopy copier(effects, volumeStream); // copies sound into i2s

unsigned long lastMillis;

// Arduino Setup
void setup(void)
{
  // Open Serial
  //Serial.begin(115200);
  // change to Warning to improve the quality
  //AudioLogger::instance().begin(Serial, AudioLogger::Info);

  // setup effects
  effects.addEffect(pitchShiftEffect);

  // start I2S out
  auto config_out = outStream.defaultConfig(TX_MODE);
  config_out.copyFrom(audioInfo);
  config_out.i2s_format = I2S_STD_FORMAT;
  config_out.is_master = true;
  config_out.port_no = 1;
  // config_out.pin_bck = I2S_SPEAKER_BCLK;
  // config_out.pin_ws = I2S_SPEAKER_LRC; // Same as I2S_SPEAKER_LCK
  // config_out.pin_data = I2S_SPEAKER_DIN;
  config_out.pin_bck = 10;
  config_out.pin_ws = 9; // Same as I2S_SPEAKER_LCK
  config_out.pin_data = 11;
  outStream.begin(config_out);

  // start I2S in
  Serial.println("starting I2S...");
  auto config_in = inStream.defaultConfig(RX_MODE);
  config_in.copyFrom(audioInfo);
  config_in.i2s_format = I2S_STD_FORMAT;
  config_in.is_master = true;
  config_in.port_no = 0;
  config_in.pin_bck = I2S_MIC_SCK;
  config_in.pin_ws = I2S_MIC_WS;
  config_in.pin_data = I2S_MIC_SD;
  // config_in.fixed_mclk = sample_rate * 256
  // config_in.pin_mck = 2
  inStream.begin(config_in);

  effects.begin(config_out);

  // set initial volume
  auto volumeConfig = volumeStream.defaultConfig();
  volumeConfig.copyFrom(audioInfo);
  volumeConfig.allow_boost = true;
  volumeStream.begin(volumeConfig); // we need to provide the bits_per_sample and channels

  Serial.println("I2S started...");

  lastMillis = millis();
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Arduino loop - copy sound to out
void loop()
{
  auto thisMillis = millis();
  if ((thisMillis - lastMillis) > 10)
  {
    auto volumeAnalogValue = analogRead(VOLUME_PIN);
    auto pitchAnalogValue = analogRead(PITCH_PIN);

    // Serial.printf("************************\n");
    // Serial.printf("volumeAnalogValue: %d\n", volumeAnalogValue);
    // Serial.printf("pitchAnalogValue: %d\n", pitchAnalogValue);

    volumeValue = mapfloat(volumeAnalogValue, 0, 4095, 0.1, 4);
    pitchValue = mapfloat(pitchAnalogValue, 0, 4095, 0.1, 4);

    // Serial.printf("volumeValue: %f\n", volumeValue);
    // Serial.printf("pitchValue: %f\n", pitchValue);

    volumeStream.setVolume(volumeValue);
    pitchShiftEffect.setValue(pitchValue);

    lastMillis = thisMillis;
  }

  copier.copy();
}
