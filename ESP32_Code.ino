//made with help from:
//https://github.com/m5stack/M5Unit-ENV/blob/master/examples/Unit_ENVIV_M5Core/Unit_ENVIV_M5Core.ino
//https://randomnerdtutorials.com/esp32-thingspeak-publish-arduino/
//https://how2electronics.com/interfacing-pms5003-air-quality-sensor-arduino/
//https://github.com/fu-hsi/PMS/blob/master/examples/Advanced/Advanced.ino
//https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/WPS/WPS.ino#L98

#include <WiFi.h>
#include "sdkconfig.h"
#include "esp_wps.h"
#include "M5UnitENV.h"
#include "PMS.h"
#include "DFRobot_OzoneSensor.h"


//WiFi Variables
WiFiMulti WiFiMulti;
const char* networkName;
const char* networkPassword;
const char* host = "api.thingspeak.com";
#define ESP_WPS_MODE WPS_TYPE_PBC

//environmental data variables

//ENV IV
SHT4X sht4;
BMP280 bmp;
float temp;
float humidity;
float pressure;
#define SEA_LEVEL_PRESSURE  1013.25f

//DFR SEN0321
float O3;
DFRobot_OzoneSensor Ozone;
#define COLLECT_NUMBER   50
#define Ozone_IICAddress OZONE_ADDRESS_3

//Plantower PMS5003
float PM1_0;
float PM2_5;
float PM10_0;
HardwareSerial mySerial(2);  // Use Serial2 (you can use Serial1 or Serial depending on your pin assignments)
PMS pms(mySerial);  // Create PMS object with HardwareSerial
PMS::DATA data;  // Create a data object to hold the sensor readings

//thingspeak api variable
const char * sendApiKey = "<add your write API key here>";


void setup() {
  //stop bluetooth and lower CPU to save power
  setCpuFrequencyMhz(80);
  btStop();
  delay(10000);
  //Serial for debugging
  Serial.begin(115200);
  //Serial for data (RX pin /TX pin)
  mySerial.begin(9600, SERIAL_8N1, 16, 17);
  
  wpsConnect();
  checkSensors();
  
}
//Loop of the code, runs through wake up of PMS, reading of all 3 sensors, sleeps PMS sensor and uploads readings
void loop() {
  wakeUp();
  readWeather();
  readO3();
  readPMS();
  sleepSensors();
  sendData();
  delay(58UL * 60UL * 1000UL);
}

void checkSensors(){
  //Setting up SHT40
  while (!sht4.begin(&Wire, SHT40_I2C_ADDR_44, 21, 22, 400000U)) {
    Serial.println("Couldn't find SHT4x");
    delay(1000);
  } Serial.println("Found SHT4x");
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  //Setting up BMP280
  while (!bmp.begin(&Wire, BMP280_I2C_ADDR, 21, 22, 400000U)) {
    Serial.println("Couldn't find BMP280");
    delay(1000);
  } Serial.println("Found BMP280");
  //default settings from datasheet for BMP
  bmp.setSampling(BMP280::MODE_NORMAL,     //Operating Mode
                  BMP280::SAMPLING_X2,     //Temp. oversampling
                  BMP280::SAMPLING_X16,    //Pressure oversampling
                  BMP280::FILTER_X16,      //Filtering
                  BMP280::STANDBY_MS_500); //Standby time

  //Setting up O3
  while(!Ozone.begin(Ozone_IICAddress)) {
    Serial.println("I2c device number error O3 Sensor !");
    delay(1000);
  }  Serial.println("I2c connect success for O3 Sensor !");
  Ozone.setModes(MEASURE_MODE_PASSIVE);

  Serial.println("got sensors ok");
}

//setNetwork and connectToNetwork functions can be used in place of WPS connect if you want to manually define
//this is recommended if you have poor WiFi signal, power outages or want to use deep sleep to optimise power
// as the ESP32 can't reconnect via WPS automatically.
void setNetwork(){
  networkName = "<insert network name>";
  networkPassword = "<insert network password>";
}

void connectToNetwork(){
  WiFiMulti.addAP(networkName, networkPassword);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

//Tell the esp32 to be looking for WPS signal
void wpsStart() {
  esp_wps_config_t config;
  memset(&config, 0, sizeof(esp_wps_config_t));
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, "ESPRESSIF");
  strcpy(config.factory_info.model_number, CONFIG_IDF_TARGET);
  strcpy(config.factory_info.model_name, "ESPRESSIF IOT");
  strcpy(config.factory_info.device_name, "ESP DEVICE");
  strcpy(config.pin, "00000000");
  esp_err_t err = esp_wifi_wps_enable(&config);
  if (err != ESP_OK) {
    Serial.printf("WPS Enable Failed: 0x%x: %s\n", err, esp_err_to_name(err));
    return;
  }

  err = esp_wifi_wps_start(0);
  if (err != ESP_OK) {
    Serial.printf("WPS Start Failed: 0x%x: %s\n", err, esp_err_to_name(err));
  }
}

void wpsStop() {
  esp_err_t err = esp_wifi_wps_disable();
  if (err != ESP_OK) {
    Serial.printf("WPS Disable Failed: 0x%x: %s\n", err, esp_err_to_name(err));
  }
}

//handling of WiFi events, will attempt to reconnect on WiFi loss if it can (may not be able to when using WPS)
//will log all wiFi events and automatically retry WPS connection if it failed or timed out
void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START: Serial.println("Station Mode Started"); break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("Connected to :" + String(WiFi.SSID()));
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("Disconnected from station, attempting reconnection");
      WiFi.reconnect();
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      Serial.println("WPS Successful, stopping WPS and connecting to: " + String(WiFi.SSID()));
      wpsStop();
      delay(10);
      WiFi.begin();
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      Serial.println("WPS Failed, retrying");
      wpsStop();
      wpsStart();
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      Serial.println("WPS Timedout, retrying");
      wpsStop();
      wpsStart();
      break;
    case ARDUINO_EVENT_WPS_ER_PIN: Serial.println("WPS_PIN = " + wpspin2string(info.wps_er_pin.pin_code)); break;
    default:                       break;
  }
}

//converts wps pin to a string
String wpspin2string(uint8_t a[]) {
  char wps_pin[9];
  for (int i = 0; i < 8; i++) {
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

//handles initialisation of WPS connection, the inbuilt LED will turn off and then back on once connection has been established
void wpsConnect(){
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.onEvent(WiFiEvent); 
  WiFi.mode(WIFI_MODE_STA);
  Serial.println("Starting WPS");
  wpsStart();
  Serial.println("Waiting for WiFi");
  digitalWrite(LED_BUILTIN, LOW);
  while (WiFi.status() != WL_CONNECTED) {
    
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

//send data function, uses an http request, send all thingspeak data in this format and you can add or remove fields following this URL format.
void sendData(){

  Serial.println("Prepare to send data");

  WiFiClient client;

  const int httpPort = 80;
  delay(10000);
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  else
  {
    String data_to_send = sendApiKey;
    data_to_send += "&field1=";
    data_to_send += String(1);
    data_to_send += "&field2=";
    data_to_send += String(temp);
    data_to_send += "&field3=";
    data_to_send += String(humidity);
    data_to_send += "&field4=";
    data_to_send += String(pressure);
    data_to_send += "&field5=";
    data_to_send += String(O3);
    data_to_send += "&field6=";
    data_to_send += String(PM1_0);
    data_to_send += "&field7=";
    data_to_send += String(PM2_5);
    data_to_send += "&field8=";
    data_to_send += String(PM10_0);
    data_to_send += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print(String("X-THINGSPEAKAPIKEY: ") + sendApiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(data_to_send.length());
    client.print("\n\n");
    client.print(data_to_send);
    delay(1000);
    while(client.available()) {
      String response = client.readString();
      Serial.println(response);
    }
  }
}

//the functions below are for reading each sensor and give (somewhat) nicely formatted serial logs
void readPMS(){
  //read the PMS5003 sensor
  Serial.println("----PMS5003----");
  // Serial.println("Send read request...");
  pms.requestRead();
  // Serial.println("Wait max. 2 seconds for read...");
  delay(2000);
  if (pms.readUntil(data)) {
    PM1_0 = data.PM_AE_UG_1_0;
    PM2_5 = data.PM_AE_UG_2_5;
    PM10_0 = data.PM_AE_UG_10_0;
    Serial.println("PM1.0 : ");
    Serial.print(PM1_0);
    Serial.print("(ug/m3)");
    Serial.println("PM2.5 : ");
    Serial.print(PM2_5);
    Serial.print("(ug/m3)");
    Serial.println("PM10.0 : ");
    Serial.print(PM10_0);
    Serial.print("(ug/m3)");
    Serial.println("-------------\r\n");
  } else {
    Serial.println("PMS5003 Read Error!");
  }
}

void readO3(){
  //read the SEN0321 unit
  Serial.println("----SEN0321----");
  O3 = Ozone.readOzoneData(COLLECT_NUMBER);
  Serial.println(O3);
  Serial.print("ppb");
  Serial.println("-------------\r\n");
}

void readWeather(){
  //read the ENV IV unit
  if (sht4.update()) {
    Serial.println("-----SHT4X-----");
    Serial.print("Temperature: ");
    temp = sht4.cTemp;
    Serial.print(temp);
    Serial.println(" degrees C");
    Serial.print("Humidity: ");
    humidity = sht4.humidity;
    Serial.print(humidity);
    Serial.println("% rH");
    Serial.println("-------------\r\n");
  }
  //check BMP sensor
  if (bmp.update()) {
    Serial.println("-----BMP280-----");
    Serial.print(F("Pressure: "));
    pressure = bmp.pressure;
    Serial.print(pressure);
    Serial.println(" Pa");
    Serial.print(F("Approx altitude: "));
    Serial.print(bmp.altitude);
    Serial.println(" m");
    Serial.println("-------------\r\n");
  }
}

//if you're using any other sensors that need waking up and sleeping they can be added here
void wakeUp(){
  //wake up any sensors that need it.
  Serial.println("Waking up, wait 120 seconds for stable readings...");
  pms.wakeUp();
  delay(120000);
}

void sleepSensors(){
  //put sensors back to sleep
  pms.sleep();
}

