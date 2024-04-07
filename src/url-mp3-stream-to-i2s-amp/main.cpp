#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"

#define I2S_SPEAKER_LRC 2
#define I2S_SPEAKER_LCK 2 // LCK same as LRC
#define I2S_SPEAKER_BCLK 3
#define I2S_SPEAKER_DIN 4

const char *WIFI_SSID = "ssid here";
const char *WIFI_PASSWORD = "password here";

URLStream url;
I2SStream i2sOutStream;                                                    // final output of decoded stream
EncodedAudioStream mp3DecoderStream(&i2sOutStream, new MP3DecoderHelix()); // Decoding stream
StreamCopy copier(mp3DecoderStream, url);                                  // copy url to decoder

void initOTA()
{
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("esp32-c3-polka");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.setTimeout(10000);

  ArduinoOTA
      .onStart([]()
               {
      Serial.println("OTA update starting.");

      Serial.println("Stopping AudioTools copier.");
      copier.end();

      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();
}

void setup()
{
  Serial.begin(115200);

  // WiFi.mode(WIFI_STA);
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // connect to WIFI
  Serial.print("Connecting to wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  initOTA();

  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  // setup i2sOutStream
  auto config = i2sOutStream.defaultConfig(TX_MODE);

  // This specific config is needed for the "Winamp & iTunes" stream at WRJQ radio: https://www.wrjqradio.com
  config.sample_rate = 22050; // 44100 / 2
  config.bits_per_sample = 32;
  config.channels = 1;
  // Or, set any time after i2sOutStream.begin() with:
  // i2sOutStream.setAudioInfo(AudioInfo(22050, 1, 32));

  config.pin_ws = I2S_SPEAKER_LCK; // Same as I2S_SPEAKER_LCK
  config.pin_bck = I2S_SPEAKER_BCLK;
  config.pin_data = I2S_SPEAKER_DIN;
  i2sOutStream.begin(config);

  // setup I2S based on sampling rate provided by decoder
  mp3DecoderStream.begin();

  // mp3 radio
  // url.begin("http://stream.srg-ssr.ch/m/rsj/mp3_128", "audio/mp3");
  url.begin("http://178.159.3.19:8665/stream", "audio/mp3"); // WRJQ radio: https://www.wrjqradio.com

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  copier.copy();
  ArduinoOTA.handle();
}
