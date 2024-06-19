#include <Arduino.h>
#include <stdint.h>
#include <DFRobotDFPlayerMini.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <TFT_eSPI.h>

const char* ssid = "绘梨衣のLAPTOP 1067"; // 替换为您的Wi-Fi SSID
const char* password = "1122334455"; // 替换为您的Wi-Fi密码
const char* API_KEY = "0a3c8aab6b56d95d03bdf22147d368ba"; // 替换为您的天气API密钥
const char* CITY = "440402"; // 替换为您想查询的位置

int status = 1;
int volume = 12;

int WeatherRXPin = 12;
int WeatherTXPin = 13;
int DFPlayerRXPin = 5;
int DFPlayerTXPin = 4;

String temperature;
String weather;

HardwareSerial myVoiceSerial(2);
HardwareSerial myDFPlayerSerial(1);

DFRobotDFPlayerMini myDFPlayer;
TFT_eSPI tft = TFT_eSPI(); // 创建TFT对象
int textWidth = tft.textWidth("Ready!") + 6; // 获取文本的宽度
int textHeight = tft.fontHeight(); // 获取文本的高度

bool initializeDFPlayer() {
  // 尝试初始化DFPlayer Mini，最多尝试10次
  int attempts = 10;
  while (attempts > 0) {
    if (myDFPlayer.begin(myDFPlayerSerial)) {
      Serial.println(F("DFPlayer Mini online."));
      myDFPlayer.volume(volume); // 设置音量
      myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD); // 设置输出设备为SD卡
      return true; // 初始化成功
    } else {
      Serial.println(F("Unable to begin:"));
      Serial.println(F("1. Please recheck the connection!"));
      Serial.println(F("2. Please insert the SD card!"));
      printDetail(myDFPlayer.readType(), myDFPlayer.read());
      delay(3000); // 稍微延迟一下再次尝试
      attempts--;
    }
  }
  Serial.println(F("Failed to initialize DFPlayer Mini after multiple attempts."));
  return false; // 初始化失败
}

void connectToWiFi(unsigned long timeout) {
  unsigned long startTime = millis();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime >= timeout) {
      Serial.println("Wi-Fi connection timed out.");
      WiFi.disconnect();
      status = 0;
      break;
    }
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("Connected to Wi-Fi");
    status = 1;
  }
}

void getWeatherData() {
  // 构建请求URL
  String url = "https://restapi.amap.com/v3/weather/weatherInfo?key=";
  url += API_KEY;
  url += "&city="; // 添加城市参数
  url += CITY;     // 连接城市名称

  // 发送HTTP请求
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    // 解析JSON数据
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (!error) {
      weather = doc["lives"][0]["weather"].as<String>();
      temperature = doc["lives"][0]["temperature"].as<String>();
      Serial.print("Weather: ");
      Serial.println(weather);
      Serial.print("Temperature: ");
      Serial.println(temperature);
    } else {
      Serial.println("JSON deserialization error.");
    }
  } else {
    Serial.println("Error fetching weather data.");
  }
  http.end();
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void handleCommand(int Command) {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  static const long interval = 3000; // 设置一个间隔时间

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    switch (Command) {
      case 0:
        getWeatherData();
        tft.fillScreen(TFT_WHITE);
        tft.drawString("Weather: " + weather, 0, 0);
        tft.drawString("Temperature: " + temperature, 0, 20);
        break;
      case 1:
        myDFPlayer.play(1);
        break;
      case 2:
        myDFPlayer.randomAll();
        tft.fillRect(100, 75, textWidth, textHeight, TFT_BLACK);
        tft.drawString("Pause!", 100, 75);
        break;
      case 3:
        myDFPlayer.start();
        break;
      case 4:
        myDFPlayer.pause();
        break;
      case 5:
        volume -= 3;
        myDFPlayer.volume(volume);
        break;
      case 6:
        volume += 3;
        myDFPlayer.volume(volume);
        break;
      case 7:
        myDFPlayer.enableLoop(); // enable loop
        break;
      case 8:
        myDFPlayer.disableLoop(); // disable loop
        break;
      case 9:
        myDFPlayer.previous();  // Play previous mp3
        break;
      case 22:
        myDFPlayer.next();  // Play next mp3
        break;
      case 23:
        myDFPlayer.loopFolder(1); // loop all mp3 files in folder SD:/01
        break;
      case 24:
        myDFPlayer.loopFolder(2); // loop all mp3 files in folder SD:/02
        break;
    }
  }
}

void setup() {
  // 初始化串口，指定引脚
  Serial.begin(9600);
  myVoiceSerial.begin(115200, SERIAL_8N1, WeatherRXPin, WeatherTXPin);
  myDFPlayerSerial.begin(9600, SERIAL_8N1, DFPlayerRXPin, DFPlayerTXPin);

  // 连接到Wi-Fi网络
  connectToWiFi(10000);

  // 初始化DFPlayer串口
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  // 尝试初始化DFPlayer Mini
  if (!initializeDFPlayer()) {
    status = 0; // 设置状态为0，表示初始化失败
  }

  // 设置音量
  myDFPlayer.volume(volume);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  // 初始化TFT屏幕
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  tft.setTextSize(2); // 设置文字大小为2
  tft.setTextColor(TFT_BLACK); // 设置文字颜色为黑色

  tft.fillScreen(TFT_WHITE);
}

void loop() {
  // 检查DFPlayer初始化状态
  if (status == 0 && !initializeDFPlayer()) {
    delay(1000); // 如果初始化失败，等待一秒再重试
    return; // 跳过此次循环的剩余部分
  }

  // 检查Wi-Fi连接状态
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi out");
    connectToWiFi(10000); // 重新尝试连接Wi-Fi
    status = 1;
  }

  // 处理DFPlayer的消息
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read());
  }

  // 处理串口命令
  if (myVoiceSerial.available()) {
    int Command = myVoiceSerial.read(); // 从电脑读取数据
    handleCommand(Command);
  }
  delay(10); // 加一个小延时，防止循环执行过快
}
