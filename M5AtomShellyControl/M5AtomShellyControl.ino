#define DECODE_NEC

#include "M5Atom.h"
#include <M5Atom.h>
#include <IRremote.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "wifi_credentials.private.h"

//Configuration
const int IR_RECEIVE_PIN = G25;
const int REPEAT_MILLIS = 250;
const long LONG_PRESS_TIME = 1000;

const String DeviceUrls[] = {
  "http://0:0@192.168.2.113/light/0/", //Woonkamer lamp
  "http://0:0@192.168.2.112/light/0/", //Eetkamer lamp
  "http://0:0@192.168.2.72/light/0/", //Keuken lamp
  "http://0:0@192.168.2.73/white/3/", //Keuken RGB
  "http://0:0@192.168.2.81/light/0/", //Buitenlamp steen
  "http://0:0@192.168.2.104/relay/0/" //Tuin LEDs
};

const byte Numbers[] = {0x52, 0x16, 0x19, 0x0D, 0x0C, 0x18, 0x5E, 0x08, 0x1C, 0x5A};


//Global variables
bool WasConnected = false;
int brightness = 0;String DeviceUrl = DeviceUrls[0];
unsigned long StartPressMillis = 0;
unsigned long RepeatMillis = 0;
uint16_t StartPressCommand = 0;

WiFiMulti wifiMulti;
HTTPClient http;

void setup() {
  M5.begin(true, false, true);
  delay(50);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  // wait for serial port to connect. Needed for native USB port only
  while (!Serial) {}

      wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("\nConnecting Wifi...\n"); 

}

void loop() {
  int connectTimeoutMs = 1000;
  bool isConnected = wifiMulti.run(connectTimeoutMs) == WL_CONNECTED;
  
  //Notify connection change
  if (isConnected != WasConnected) {
    WasConnected = isConnected;
    if (isConnected) {
      Serial.print("WiFi connected: ");
      Serial.print(WiFi.SSID());
      Serial.print(" ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi connection lost!");
    }
  }

  if (isConnected) {
    if (IrReceiver.decode()) {
      ExecuteIRCommand();
    } else {
      if (StartPressMillis > 0 && millis() - RepeatMillis > REPEAT_MILLIS) {
        StartPressMillis = 0;
        Serial.print("Stop press millis at ");
        Serial.println( millis() );
      }
    }
    IrReceiver.resume();
  }
}

void ExecuteIRCommand() {

  // https://github.com/Arduino-IRremote/Arduino-IRremote#decodedirdata-structure

  // Check if the buffer overflowed
  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    Serial.println(F("Try to increase the RAW_BUFFER_LENGTH"));
    return;
  }
  if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
    //Serial.println("Received noise or an unknown (or not yet enabled) protocol");
    return;
  }

  //IrReceiver.printIRResultShort(&Serial);  

  bool isRepeat = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
  uint16_t command = IrReceiver.decodedIRData.command;
  //Serial.print("Receive: ");  Serial.print( command );  if (isRepeat) { Serial.print(" REPEAT"); } Serial.println();

  unsigned long now = millis();
  if (!isRepeat) {
    StartPressCommand = command;
    StartPressMillis = now;
    Serial.print("StartPress at "); Serial.println( StartPressMillis);
  }
 

  bool isLongPress = now - StartPressMillis >= LONG_PRESS_TIME && RepeatMillis - StartPressMillis < LONG_PRESS_TIME;
  bool isNumber = command != 0x52 && ArrayContains(command, Numbers, sizeof(Numbers) / sizeof(Numbers[0])); //1 .. 9
  
  if (isLongPress) {
    Serial.println(RepeatMillis - StartPressMillis);
    Serial.println(" LONG PRESS");
  }
   RepeatMillis = now;

//0x40=OK, 0x43=Right, 0x44=Left, 0x46=Up, ox15=Down
//0x52=0, 0x16=1, 0x19=2, 0x0D=3, 0x0C=4, 0x18=5, 0x5E=6, 0x08=7, 0x1C=8, 0x5A=9
//0x42="*", 0x4A="#"
  if (!isRepeat) {
    //TODO: Long press of the numeric keys must toggle the state
    if (command == 0x16) DeviceUrl = DeviceUrls[0];  // 1
    if (command == 0x19) DeviceUrl = DeviceUrls[1];  // 2
    if (command == 0x0d) DeviceUrl = DeviceUrls[2];  // 3
    if (command == 0x0c) DeviceUrl = DeviceUrls[3];  // 4
    if (command == 0x18) DeviceUrl = DeviceUrls[4];  // 5
    if (command == 0x5e) DeviceUrl = DeviceUrls[5];  // 6
    //if (command == 0x08) DeviceUrl = DeviceUrls[6];  // 7
    //if (command == 0x1c) DeviceUrl = DeviceUrls[7];  // 8
    //if (command == 0x5a) DeviceUrl = DeviceUrls[8];  // 9

    if (command == 0x40) ToggleLampje(DeviceUrl); // "OK"

    // "*" min brightness
    if (command == 0x42) {
      brightness = 1;
      SetBrightness(DeviceUrl, brightness);
    }

    // "0" 50% brightness
    if (command == 0x52) {
      brightness = 50;
      SetBrightness(DeviceUrl, brightness);
    }

    // "#" max brightness
    if (command == 0x4A) {
      brightness = 100;
      SetBrightness(DeviceUrl, brightness);
    }
  }

  //Long press number
  if (isLongPress && isNumber) {
    ToggleLampje(DeviceUrl);
  }

  //Dim down
  if (command == 0x15) {
    if (!isRepeat) {
      brightness = GetBrightness(DeviceUrl);
    }
    brightness = max(1, brightness - 3);
    SetBrightness(DeviceUrl, brightness);
  }

  //Dim up
  if (command == 0x46) {
    if (!isRepeat) {
      brightness = GetBrightness(DeviceUrl);
    }
    brightness = min(100, brightness + 3);
    SetBrightness(DeviceUrl, brightness);
  }

  // M5.update();    
  // if (M5.Btn.isPressed()) {
  //    Serial.println("Button is pressed.");
  // }

}

int GetBrightness(String url) {
    String json = GetUrl(url);
    return GetJsonIntVal(json, "brightness"); 
}

void SetBrightness(String url, int brightness)
{
  GetUrl(url + "?brightness=" + brightness);
}

void ToggleLampje(String url)
{
  String json = GetUrl(url + "?turn=toggle");
}

bool ArrayContains(byte val, const byte arr[], byte n) {
  for(size_t i = 0; i < n; i++) {
        if(arr[i] == val)
            return true;
    }
    return false;
}

String GetUrl(String url) {
  String payload = "";
  Serial.print("[HTTP] ");
  Serial.print(url);
  Serial.print("\n");
  http.begin(url);  // configure traged server and
  int httpCode = http.GET();  // start connection and send HTTP header.
  if (httpCode > 0) {  // httpCode will be negative on error.
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK) {  // file found at server.
          payload = http.getString();
          Serial.println(payload);
      }
  } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n",
                    http.errorToString(httpCode).c_str());
      payload="";
  }

  http.end(); 
  return payload;
}

int GetJsonIntVal(String input_json, String property_name) {
  StaticJsonDocument<200> filter;
  filter[property_name] = true;

  // Deserialize the document
  StaticJsonDocument<400> doc;
  deserializeJson(doc, input_json, DeserializationOption::Filter(filter));

  int value = doc[property_name];
  Serial.print(property_name);
  Serial.print(" ");
  Serial.println(value);
  // Print the result
  serializeJsonPretty(doc, Serial);
  return value;
}
