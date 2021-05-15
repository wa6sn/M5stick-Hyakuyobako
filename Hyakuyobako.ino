#include <M5StickCPlus.h>
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>
#include "SHT3X.h"
#include "MHZ19_uart.h"
#include "Ambient.h"

#define DISP_WIDTH  240
#define DISP_HEIGHT 135
#define BRIGHTNESS 10

#define LCD_MODE_DIGIT     0
#define LCD_MODE_TMP_GRAPH 1
#define LCD_MODE_HUM_GRAPH 2
#define LCD_MODE_PRE_GRAPH 3
#define LCD_MODE_CO2_GRAPH 4

#define RX_PIN 26                     // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 0                      // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                 // Device to MH-Z19 Serial baudrate (should not be changed)

#define PERIOD 60

SHT3X sht30;
Adafruit_BMP280 bme;
MHZ19_uart mhz19;

WiFiClient client;
Ambient ambient;

const char* ssid = "XXXXXXXXXXXXXXX";
const char* password = "XXXXXXXXXXXXXXX";

unsigned int channelId = XXXXXXXXXXXXXXX; // AmbientのチャネルID
const char* writeKey = "XXXXXXXXXXXXXXX"; // ライトキー

const int X_SPACE = 6;
const int Y_SPACE = 5;
const int RECT_WIDTH  = (240 - 3*X_SPACE)/2;
const int RECT_HEIGHT = (135 - 3*Y_SPACE)/2;

int lcdMode = LCD_MODE_DIGIT;

unsigned long getDataTimer = 0;

float tmp_history[DISP_WIDTH] = {};
float hum_history[DISP_WIDTH] = {};
float pre_history[DISP_WIDTH] = {};
float co2_history[DISP_WIDTH] = {};

int tmp_historyPos = 0;
int hum_historyPos = 0;
int pre_historyPos = 0;
int co2_historyPos = 0;

float tmp = 0;
float hum = 0;
float pre = 0;
float co2 = 0;

void setup() {
  M5.begin();

  M5.Axp.ScreenBreath(BRIGHTNESS);
  
  Serial.begin(9600);  // Device to serial monitor feedback
  Serial.println(F("ENV Unit and HAT (SHT30 and BMP280) ..."));

  Wire.begin(32,33); // for ENV II Unit

  WiFi.begin(ssid, password);  //  Wi-Fi APに接続
  while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
    Serial.println(F("Connection attempt..."));
    delay(500);
  }

  Serial.print(F("WiFi connected\r\nIP address: "));
  Serial.println(WiFi.localIP());

  while (!bme.begin(0x76)){  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    M5.Lcd.println("Could not find a valid BMP280 sensor, check wiring!");
  }

  Serial.println(F("prepare MH-Z19C Hat..."));
  mhz19.begin(RX_PIN, TX_PIN);
  mhz19.setAutoCalibration(false);

  ambient.begin(channelId, writeKey, &client);
  
  delay(3000);

  M5.Lcd.setRotation(1);
  M5.Lcd.setTextFont(1);
  
  M5.Lcd.fillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, BLACK);
  render();
}

void loop() {
  auto now = millis();

  M5.update();
  if ( M5.BtnA.wasPressed() ) {
    lcdMode = (lcdMode + 1) % 5;
    render();
  }
  
  if (now - getDataTimer >= PERIOD*1000) {
    /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even
      if below background CO2 levels or above range (useful to validate sensor). You can use the
      usual documented command with getCO2(false) */

    pre = bme.readPressure() / 100;
    if(sht30.get()==0){
      tmp = sht30.cTemp;
      hum = sht30.humidity;
    }
    co2 = mhz19.getPPM();
    
    Serial.printf("Temp: %2.2f*C  Hum: %0.2f%%  Pres: %0.2fhPa  CO2ppm: %4.0f\r\n", tmp, hum, pre, co2);
    
    // 測定結果の表示
    tmp_historyPos = (tmp_historyPos + 1) % (sizeof(tmp_history) / sizeof(int));
    hum_historyPos = (hum_historyPos + 1) % (sizeof(hum_history) / sizeof(int));
    pre_historyPos = (pre_historyPos + 1) % (sizeof(pre_history) / sizeof(int));
    co2_historyPos = (co2_historyPos + 1) % (sizeof(co2_history) / sizeof(int));
    
    tmp_history[tmp_historyPos] = tmp;
    hum_history[hum_historyPos] = hum;
    pre_history[pre_historyPos] = pre;
    co2_history[co2_historyPos] = co2;
    
    render();

    // 温度、湿度、気圧、CO2 の値をAmbientに送信する
    ambient.set(1, String(tmp).c_str());
    ambient.set(2, String(hum).c_str());
    ambient.set(3, String(pre).c_str());
    ambient.set(4, String(co2).c_str());
    
    ambient.send();

    getDataTimer = now;
  }

}

void render() {
  // clear
  M5.Lcd.fillRect(0, 0, DISP_WIDTH, DISP_HEIGHT, BLACK);

  switch (lcdMode) {
    case LCD_MODE_DIGIT:
    
      // | 1 | 2 | 
      // | 3 | 4 |
    
      // rect 1
      M5.Lcd.drawRect(X_SPACE, Y_SPACE, RECT_WIDTH, RECT_HEIGHT, WHITE);
      
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(X_SPACE+1, Y_SPACE+1);
      M5.Lcd.setTextColor(BLACK, WHITE);
      M5.Lcd.printf("Temp");
    
      M5.Lcd.setTextSize(4);
      M5.Lcd.setCursor(15, 30);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.printf("%2.1f", tmp_history[tmp_historyPos]);
    
      // rect 2
      M5.Lcd.drawRect(X_SPACE*2+RECT_WIDTH, Y_SPACE, RECT_WIDTH, RECT_HEIGHT, WHITE);
    
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(X_SPACE*2+RECT_WIDTH+1, Y_SPACE+1);
      M5.Lcd.setTextColor(BLACK, WHITE);
      M5.Lcd.printf("Humid");
    
      M5.Lcd.setTextSize(4);
      M5.Lcd.setCursor(180, 30);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.printf("%2.0f", hum_history[hum_historyPos]);
    
      // rect 3
      M5.Lcd.drawRect(X_SPACE, Y_SPACE*2+RECT_HEIGHT, RECT_WIDTH, RECT_HEIGHT, WHITE);
    
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(BLACK, WHITE);
      M5.Lcd.setCursor(X_SPACE+1, Y_SPACE*2+RECT_HEIGHT+1);
      M5.Lcd.printf("Pres");
      
      M5.Lcd.setTextSize(4);
      M5.Lcd.setCursor(15, 95);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.printf("%4.0f", pre_history[pre_historyPos]);
    
      // rect 4
      M5.Lcd.drawRect(X_SPACE*2+RECT_WIDTH, Y_SPACE*2+RECT_HEIGHT, RECT_WIDTH, RECT_HEIGHT, WHITE);
    
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(BLACK, WHITE);
      M5.Lcd.setCursor(X_SPACE*2+RECT_WIDTH+1, Y_SPACE*2+RECT_HEIGHT+1);
      M5.Lcd.printf("CO2");
    
      M5.Lcd.setTextSize(4);
      M5.Lcd.setCursor(132, 95);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.printf("%4.0f", co2_history[co2_historyPos]);
      
      break;
      
    case LCD_MODE_TMP_GRAPH:

      for (int i = 1; i < DISP_WIDTH; i++) {
        auto value = max(0, (int)(tmp_history[(tmp_historyPos + 1 + i) % DISP_WIDTH]));
        auto y = min(DISP_HEIGHT, (int)(value * (DISP_HEIGHT / 40.0)));
        
        auto pvalue = max(0, (int)(tmp_history[(tmp_historyPos + i) % DISP_WIDTH]));
        auto py = min(DISP_HEIGHT, (int)(pvalue * (DISP_HEIGHT / 40.0)));
        
        M5.Lcd.drawLine(i-1, DISP_HEIGHT - y, i, DISP_HEIGHT - py, WHITE);
      }

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(5, 5);
      M5.Lcd.printf("Temperature:\n 0-40 cer.");
      M5.Lcd.drawLine(0, DISP_HEIGHT/2, DISP_WIDTH, DISP_HEIGHT/2, RED);
      break;
      
    case LCD_MODE_HUM_GRAPH:
      for (int i = 1; i < DISP_WIDTH; i++) {
        auto value = max(0, (int)(hum_history[(hum_historyPos + 1 + i) % DISP_WIDTH]));
        auto y = min(DISP_HEIGHT, (int)(value * (DISP_HEIGHT / 100.0)));

        auto pvalue = max(0, (int)(hum_history[(hum_historyPos + i) % DISP_WIDTH]));
        auto py = min(DISP_HEIGHT, (int)(pvalue * (DISP_HEIGHT / 100.0)));
        
        M5.Lcd.drawLine(i-1, DISP_HEIGHT - y, i, DISP_HEIGHT - py, WHITE);
      }

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(5, 5);
      M5.Lcd.printf("Humidity:\n 0-100 pct.");
      M5.Lcd.drawLine(0, DISP_HEIGHT/2, DISP_WIDTH, DISP_HEIGHT/2, RED);
      break;
      
    case LCD_MODE_PRE_GRAPH:
      for (int i = 1; i < DISP_WIDTH; i++) {
        auto value = max(0, (int)(pre_history[(pre_historyPos + 1 + i) % DISP_WIDTH]) - 900);
        auto y = min(DISP_HEIGHT, (int)(value * (DISP_HEIGHT / 200.0)));
        
        auto pvalue = max(0, (int)(pre_history[(pre_historyPos + i) % DISP_WIDTH]) - 900);
        auto py = min(DISP_HEIGHT, (int)(pvalue * (DISP_HEIGHT / 200.0)));
        
        M5.Lcd.drawLine(i-1, DISP_HEIGHT - y, i, DISP_HEIGHT - py, WHITE);
      }

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(5, 5);
      M5.Lcd.printf("Pressure:\n 900-1100 hPa");
      M5.Lcd.drawLine(0, DISP_HEIGHT/2, DISP_WIDTH, DISP_HEIGHT/2, RED);
      break;
      
    case LCD_MODE_CO2_GRAPH:
      for (int i = 1; i < DISP_WIDTH; i++) {
        auto value = max(0, (int)(co2_history[(co2_historyPos + 1 + i) % DISP_WIDTH]) - 400);
        auto y = min(DISP_HEIGHT, (int)(value * (DISP_HEIGHT / 1200.0)));
        
        auto pvalue = max(0, (int)(co2_history[(co2_historyPos + i) % DISP_WIDTH]) - 400);
        auto py = min(DISP_HEIGHT, (int)(pvalue * (DISP_HEIGHT / 1200.0)));
        
        M5.Lcd.drawLine(i-1, DISP_HEIGHT - y, i, DISP_HEIGHT - py, WHITE);
      }
      
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(5, 5);
      M5.Lcd.printf("CO2 con.:\n 400-1600 ppm");
      M5.Lcd.drawLine(0, DISP_HEIGHT/2, DISP_WIDTH, DISP_HEIGHT/2, RED);
      break;
  }
  
}
