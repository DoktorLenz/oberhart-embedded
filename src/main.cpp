/*
  RadioLib LoRaWAN End Device Example

  This example joins a LoRaWAN network and will send
  uplink packets. Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  NOTE: LoRaWAN requires storing some parameters persistently!
        RadioLib does this by using EEPROM, by default
        starting at address 0 and using 32 bytes.
        If you already use EEPROM in your application,
        you will have to either avoid this range, or change it
        by setting a different start address by changing the value of
        RADIOLIB_HAL_PERSISTENT_STORAGE_BASE macro, either
        during build or in src/BuildOpt.h.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

SPIClass spi(VSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);

// SX1278 has the following connections:
// NSS pin:   18
// DIO0 pin:  26
// RESET pin: 14
// DIO1 pin:  33
SX1276 radio = new Module(SS, DIO0, RST, 33, spi, spiSettings);

// create the node instance on the EU-868 band
// using the radio module and the encryption key
// make sure you are using the correct band
// based on your geographical location!
LoRaWANNode node(&radio, &EU868);

void setup()
{
  Serial.begin(115200);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1276] Initializing ... "));
  spi.begin(SCK, MISO, MOSI, SS);
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  // first we need to initialize the device storage
  // this will reset all persistently stored parameters
  // NOTE: This should only be done once prior to first joining a network!
  //       After wiping persistent storage, you will also have to reset
  //       the end device in TTN and perform the join procedure again!
  // node.wipe();

  // application identifier - pre-LoRaWAN 1.1.0, this was called appEUI
  // when adding new end device in TTN, you will have to enter this number
  // you can pick any number you want, but it has to be unique
  uint64_t joinEUI = 0x0;

  // device identifier - this number can be anything
  // when adding new end device in TTN, you can generate this number,
  // or you can set any value you want, provided it is also unique
  uint64_t devEUI = 0x0;

  // select some encryption keys which will be used to secure the communication
  // there are two of them - network key and application key
  // because LoRaWAN uses AES-128, the key MUST be 16 bytes (or characters) long

  // network key is the ASCII string "topSecretKey1234"
  uint8_t nwkKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // application key is the ASCII string "aDifferentKeyABC"
  uint8_t appKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // prior to LoRaWAN 1.1.0, only a single "nwkKey" is used
  // when connecting to LoRaWAN 1.0 network, "appKey" will be disregarded
  // and can be set to NULL

  // some frequency bands only use a subset of the available channels
  // you can set the starting channel and their number
  // for example, the following corresponds to US915 FSB2 in TTN
  /*
    node.startChannel = 8;
    node.numChannels = 8;
  */

  // now we can start the activation
  // this can take up to 20 seconds, and requires a LoRaWAN gateway in range
  Serial.print(F("[LoRaWAN] Attempting over-the-air activation ... "));
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  // after the device has been activated,
  // network can be rejoined after device power cycle
  // by calling "begin"
  /*
    Serial.print(F("[LoRaWAN] Resuming previous session ... "));
    state = node.begin();
    if(state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      while(true);
    }
  */
}

// counter to keep track of transmitted packets
int count = 0;

void loop()
{
  // send uplink to port 10
  Serial.print(F("[LoRaWAN] Sending uplink packet ... "));
  String strUp = "Hello World! #" + String(count++);
  int state = node.uplink(strUp, 10);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // after uplink, you can call downlink(),
  // to receive any possible reply from the server
  // this function must be called within a few seconds
  // after uplink to receive the downlink!
  Serial.print(F("[LoRaWAN] Waiting for downlink ... "));
  String strDown;
  state = node.downlink(strDown);
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));

    // print data of the packet (if there are any)
    Serial.print(F("[LoRaWAN] Data:\t\t"));
    if (strDown.length() > 0)
    {
      Serial.println(strDown);
    }
    else
    {
      Serial.println(F("<MAC commands only>"));
    }

    // print RSSI (Received Signal Strength Indicator)
    Serial.print(F("[LoRaWAN] RSSI:\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // print SNR (Signal-to-Noise Ratio)
    Serial.print(F("[LoRaWAN] SNR:\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    // print frequency error
    Serial.print(F("[LoRaWAN] Frequency error:\t"));
    Serial.print(radio.getFrequencyError());
    Serial.println(F(" Hz"));
  }
  else if (state == RADIOLIB_ERR_RX_TIMEOUT)
  {
    Serial.println(F("timeout!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait before sending another packet
  delay(10000);
}