#include <Arduino.h>
#include "AudioTools.h"

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

#define I2S_SPEAKER_LRC 9
#define I2S_SPEAKER_LCK 9 // LCK same as LRC
#define I2S_SPEAKER_BCLK 10
#define I2S_SPEAKER_DIN 11

#define VOLUME_PIN 4
#define PITCH_PIN 2

float pitchValue = 1.0;
float volumeValue = 1.0;

int sampleRate = 16000;
int channels = 1;
int bitsPerChannel = 16;

AudioInfo audioInfo(sampleRate, channels, bitsPerChannel);
I2SStream inStream;
CallbackStream intermediateStream;
I2SStream outStream;
// MultiOutput multiOutput;
StreamCopy copier(intermediateStream, inStream); // copies sound into i2s

unsigned long lastMillis;

size_t intermediateStream_cb_read(uint8_t *data, size_t len)
{
  Serial.println("intermediateStream_cb_read...");
  return len;
}

// Function to calculate autocorrelation
float autocorrelation(int16_t *buffer, int bufferSize, int lag)
{
  float sum = 0;
  for (int i = 0; i < bufferSize - lag; ++i)
  {
    sum += buffer[i] * buffer[i + lag];
  }
  return sum;
}

// Function to find pitch using autocorrelation method
float findPitch(int16_t *buffer, int bufferSize, int sampleRate)
{
  const float minFreq = 80;  // Minimum frequency of human voice
  const float maxFreq = 400; // Maximum frequency of human voice

  // Find minimum and maximum lag corresponding to minFreq and maxFreq
  int minLag = sampleRate / maxFreq;
  int maxLag = sampleRate / minFreq;

  float maxCorrelation = 0;
  int pitch = 0;

  // Iterate through lags and find the lag with maximum autocorrelation
  for (int lag = minLag; lag <= maxLag; ++lag)
  {
    float correlation = autocorrelation(buffer, bufferSize, lag);
    if (correlation > maxCorrelation)
    {
      maxCorrelation = correlation;
      pitch = lag;
    }
  }

  // Convert pitch lag to frequency
  float pitchFreq = (float)sampleRate / pitch;
  return pitchFreq;
}

size_t intermediateStream_cb_update(uint8_t *data, size_t len)
{
  // Serial.printf("intermediateStream_cb_update: len: %d\n", len);

  // for (int i = 0; i < I2S_BUFFER_SIZE; i++)
  // {
  //   Serial.printf("%02X", data[i]);
  // }
  // Serial.println();

  double samplingRate = static_cast<double>(sampleRate);

  // Sample PCM buffer (16-bit integers)
  int16_t *pcmBuffer = (int16_t*)data;
  int bufferSize = len / 2;

  // Set sample rate (you can adjust this according to your requirements)
  int sampleRate = samplingRate; // Default sample rate

  // Find pitch
  float pitch = findPitch(pcmBuffer, bufferSize, sampleRate);

  Serial.printf("Detected pitch: %f Hz\n", pitch);

  // Serial.printf("intermediateStream_cb_update: samplingRate: %f, len: %d, pitch_period: %f\n", samplingRate, len, pitch_period);

  return len;
}

// Arduino Setup
void setup(void)
{
  // delay(7000);

  // Open Serial
  Serial.begin(115200);
  // change to Warning to improve the quality
  AudioLogger::instance().begin(Serial, AudioLogger::Error);

  intermediateStream.setReadCallback(intermediateStream_cb_read);
  intermediateStream.setUpdateCallback(intermediateStream_cb_update);
  intermediateStream.setOutput(outStream);
  intermediateStream.begin(audioInfo);

  // start I2S out
  auto config_out = outStream.defaultConfig(TX_MODE);
  config_out.copyFrom(audioInfo);
  config_out.buffer_size = 1024;
  config_out.i2s_format = I2S_STD_FORMAT;
  config_out.is_master = true;
  config_out.port_no = 1;
  // config_out.pin_bck = I2S_SPEAKER_BCLK;
  // config_out.pin_ws = I2S_SPEAKER_LRC; // Same as I2S_SPEAKER_LCK
  // config_out.pin_data = I2S_SPEAKER_DIN;
  config_out.pin_bck = I2S_SPEAKER_BCLK;
  config_out.pin_ws = I2S_SPEAKER_LRC; // Same as I2S_SPEAKER_LCK
  config_out.pin_data = I2S_SPEAKER_DIN;
  outStream.begin(config_out);

  // start I2S in
  Serial.println("starting I2S...");
  auto config_in = inStream.defaultConfig(RX_MODE);
  config_in.copyFrom(audioInfo);
  config_in.buffer_size = 1024;
  config_in.i2s_format = I2S_STD_FORMAT;
  config_in.is_master = true;
  config_in.port_no = 0;
  config_in.pin_bck = I2S_MIC_SCK;
  config_in.pin_ws = I2S_MIC_WS;
  config_in.pin_data = I2S_MIC_SD;
  config_in.sample_rate = sampleRate;
  // config_in.fixed_mclk = sample_rate * 256
  // config_in.pin_mck = 2
  inStream.begin(config_in);

  Serial.println("I2S started...");

  lastMillis = millis();
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t buffer[1024];
int16_t *samples = (int16_t *)buffer;

// Arduino loop - copy sound to out
void loop()
{
  // size_t bytes_read = inStream.readBytes(buffer, I2S_BUFFER_SIZE);
  // size_t samples_read = bytes_read / sizeof(int16_t);

  auto thisMillis = millis();
  if ((thisMillis - lastMillis) > 10)
  {
    lastMillis = thisMillis;
  }

  copier.copy();
}
