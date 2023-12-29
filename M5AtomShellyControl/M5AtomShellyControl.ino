#include "M5Atom.h"
#include <M5Atom.h>

#include "config_remote.h"
#include "config_shelly.h"
#include "config_wifi.h"

#include <IRremote.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


const byte IR_Numbers[] = {
  IR_CMD_0, IR_CMD_1, IR_CMD_2, IR_CMD_3, IR_CMD_4, IR_CMD_5, IR_CMD_6, IR_CMD_7, IR_CMD_8, IR_CMD_9
};

//Global variables
bool WasConnected = false;
int brightness = 0;
String DeviceUrl = DeviceUrls[0];
unsigned long StartPressMillis = 0;
uint16_t StartPressCommand = 0;
bool IsLongPressRegistered = false;

WiFiMulti wifiMulti;
HTTPClient http;

void setup() {
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xff0000);

  while (!Serial) {} // wait for serial port to connect. Needed for native USB port only

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD); 
}

void loop() {
  if (!WifiConnection()) return;

  if (IrReceiver.decode()) {
    ExecuteIRCommand();
  }
  IrReceiver.resume();
}

/***
 * Keep WIFI alive and update status LED to RED or GREEN
 * Returns true if connection is valid
 */
bool WifiConnection() {
  bool isConnected = wifiMulti.run(WIFI_TIMEOUT_MILLIS) == WL_CONNECTED;
  
  //Notify connection change
  if (isConnected != WasConnected) {
    WasConnected = isConnected;
    if (isConnected) {
      M5.dis.drawpix(0, 0x00ff00);
      Serial.print("WiFi connected: ");
      Serial.print(WiFi.SSID());
      Serial.print(" ");
      Serial.println(WiFi.localIP());
    } else {
      M5.dis.drawpix(0, 0xff0000);
      Serial.println("WiFi connection lost!");
    }
  }
  return isConnected;
}

void ExecuteIRCommand() {

  // See https://github.com/Arduino-IRremote/Arduino-IRremote#decodedirdata-structure

  if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    //Serial.println("Received noise or an unknown (or not yet enabled) protocol");
    return;
  }

  // Check if the buffer overflowed
  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    Serial.println(F("Try to increase the RAW_BUFFER_LENGTH"));
    return;
  }

  IrReceiver.printIRResultShort(&Serial);  

  bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
  uint16_t command = IrReceiver.decodedIRData.command;
  int numberPressed = ArrayIndexOf(command, IR_Numbers, sizeof(IR_Numbers) / sizeof(IR_Numbers[0])); //0 .. 9

  //Long press detection
  unsigned long now = millis();
  if (!isRepeat) {
    IsLongPressRegistered = false;
    StartPressCommand = command;
    StartPressMillis = now;
    Serial.printf("StartPress at %d\n", StartPressMillis);
  }
  bool isLongPress = now - StartPressMillis >= LONG_PRESS_MILLIS && !IsLongPressRegistered;
  if (isLongPress) {
    Serial.println("LongPress");
    IsLongPressRegistered = true;
  }

  if (!isRepeat) {
    if (numberPressed >=1 && numberPressed <= N_Devices) {
      DeviceUrl = DeviceUrls[numberPressed - 1];
    }

    switch (command) {
      case IR_CMD_OK:
        TogglePower(DeviceUrl);
        break;

      case IR_CMD_STAR:
        brightness = DIM_MINIMUM;
        SetBrightness(DeviceUrl, brightness);
        break;
      case IR_CMD_0:
        brightness = DIM_MIDDLE;
        SetBrightness(DeviceUrl, brightness);
        break;

      case IR_CMD_HASH:
        brightness = DIM_MAXIMUM;
        SetBrightness(DeviceUrl, brightness);
        break;
    }
  }

  //Long press number
  if (isLongPress && numberPressed >= 1 && numberPressed <= N_Devices) {
    TogglePower(DeviceUrl);
  }

  //Dim down
  if (command == IR_CMD_DOWN || command == IR_CMD_LEFT) {
    if (!isRepeat) {
      brightness = GetBrightness(DeviceUrl);
    }
    brightness = max(DIM_MINIMUM, brightness - DIM_STEPS);
    SetBrightness(DeviceUrl, brightness);
  }

  //Dim up
  if (command == IR_CMD_UP || command == IR_CMD_RIGHT) {
    if (!isRepeat) {
      brightness = GetBrightness(DeviceUrl);
    }
    brightness = min(DIM_MAXIMUM, brightness + DIM_STEPS);
    SetBrightness(DeviceUrl, brightness);
  }

  // M5.update();    
  // if (M5.Btn.isPressed()) Serial.println("Button is pressed.");
}

int GetBrightness(String url) {
    String json = GetUrl(url);
    return GetJsonIntVal(json, "brightness"); 
}

void SetBrightness(String url, int brightness)
{
  GetUrl(url + "?brightness=" + brightness);
}

void TogglePower(String url)
{
  String json = GetUrl(url + "?turn=toggle");
}

size_t ArrayIndexOf(byte val, const byte arr[], byte n) {
  for(size_t i = 0; i < n; i++) {
    if(arr[i] == val) return i;
  }
  return -1;
}

/***
 * GET Request. Return the payload or empty string on failure.
 */
String GetUrl(String url) {
  String payload = "";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {  // httpCode will be negative on error.
      if (httpCode == HTTP_CODE_OK) {
          payload = http.getString();
          //Serial.println(payload);
      } else {
        Serial.printf("[HTTP] GET... unexpected status code: %d\n", httpCode);  
      }
  } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      payload="";
  }

  http.end(); 
  return payload;
}

int GetJsonIntVal(String input_json, String property_name) {
  //filter the result to reduce memory usage
  StaticJsonDocument<200> filter;
  filter[property_name] = true;

  // Deserialize the document
  StaticJsonDocument<200> doc;
  deserializeJson(doc, input_json, DeserializationOption::Filter(filter));
  // serializeJsonPretty(doc, Serial);

  int value = doc[property_name];
  return value;
}
