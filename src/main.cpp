#include <M5StickC.h>
#include <WiFiManager.h>
#include <ArduinoOSC.h>
#include <endian.h>

int duty = 25;

#define MOTOR1_DIR 26
#define MOTOR1_PWM 33
const int MOTOR1_PWMCH = 1;

#define MOTOR2_DIR 0
#define MOTOR2_PWM 32
const int MOTOR2_PWMCH = 2;

WiFiManager wifiManager;

const char* localhost = "127.0.0.1";
const int bind_port = 54345;

// ステータス
struct DEVICE_STATUS {
  float STATUS;
};
DEVICE_STATUS status;

#define BREAK     0.0f
#define FORWARD   1.0f
#define REVERSE   2.0f
#define LEFT      3.0f
#define RIGHT     4.0f

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setupWiFi() {
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.autoConnect("M5-ZAKRELLO");
}

void setup() {
  Serial.begin(115200);
  status.STATUS = BREAK;

  M5.begin();
  M5.Axp.ScreenBreath(8);   // 7-12
  M5.Lcd.setRotation(3);    // 0-3
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  ledcSetup(MOTOR1_PWMCH, 490, 8);
  ledcAttachPin(MOTOR1_PWM, MOTOR1_PWMCH);
  ledcWrite(MOTOR1_PWMCH, 0);
  
  ledcSetup(MOTOR2_PWMCH, 490, 8);
  ledcAttachPin(MOTOR2_PWM, MOTOR2_PWMCH);
  ledcWrite(MOTOR2_PWMCH, 0);

  pinMode(MOTOR1_DIR, OUTPUT);
  digitalWrite(MOTOR1_DIR, LOW);

  pinMode(MOTOR2_DIR, OUTPUT);
  digitalWrite(MOTOR2_DIR, LOW);
  
  // WiFI マネージャー初期化
  setupWiFi();
  
  Serial.println();
  // IPとポートを表示
  IPAddress localIP = WiFi.localIP();
  Serial.print(localIP.toString()); Serial.print(":"); Serial.println(bind_port);
  
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.printf("%s:%d\r\n", localIP.toString().c_str(), bind_port );

  // OSC受信設定
  OscWiFi.subscribe(bind_port, "/status/duty",
    [](const OscMessage& m)
    {
      duty = m.arg<float>(0);
      if (status.STATUS != BREAK){
        ledcWrite(MOTOR1_PWMCH, duty);
        ledcWrite(MOTOR2_PWMCH, duty);
      }     
    }
  );

  OscWiFi.subscribe(bind_port, "/status/motor",
      [](const OscMessage& m)
      {
        float c = m.arg<float>(0);
        status.STATUS = c;
        Serial.print("/status/motor "); Serial.println(c);
        if (c == FORWARD) {
          Serial.println("FORWARD");
          M5.Lcd.fillScreen(GREEN);
          ledcWrite(MOTOR1_PWMCH, duty);
          digitalWrite(MOTOR1_DIR, HIGH);
          ledcWrite(MOTOR2_PWMCH, duty);
          digitalWrite(MOTOR2_DIR, HIGH);
        }
        else if (c == REVERSE){
          Serial.println("REVERSE");
          M5.Lcd.fillScreen(RED);
          ledcWrite(MOTOR1_PWMCH, duty);
          digitalWrite(MOTOR1_DIR, LOW);
          ledcWrite(MOTOR2_PWMCH, duty);
          digitalWrite(MOTOR2_DIR, LOW);
        }
        else if (c == BREAK) {
          Serial.println("BREAK");
          M5.Lcd.fillScreen(BLACK);
          ledcWrite(MOTOR1_PWMCH, 0);
          digitalWrite(MOTOR1_DIR, LOW);
          ledcWrite(MOTOR2_PWMCH, 0);
          digitalWrite(MOTOR2_DIR, LOW);
        }
        else if (c == LEFT) {
          Serial.println("LEFT");
          M5.Lcd.fillScreen(CYAN);
          ledcWrite(MOTOR1_PWMCH, duty);
          digitalWrite(MOTOR1_DIR, HIGH);
          ledcWrite(MOTOR2_PWMCH, 0);
          digitalWrite(MOTOR2_DIR, LOW);
        }
        else if (c == RIGHT) {
          Serial.println("RIGHT");
          M5.Lcd.fillScreen(YELLOW);
          ledcWrite(MOTOR1_PWMCH, 0);
          digitalWrite(MOTOR1_DIR, LOW);          
          ledcWrite(MOTOR2_PWMCH, duty);
          digitalWrite(MOTOR2_DIR, HIGH);
        }                
      }
  );
}

void loop() {
  // Serial.println("loop");
  M5.update();
  if ( M5.BtnA.wasPressed() ) {
    Serial.println("wasPressed");
    // テストで自分に送る
    OscWiFi.send(localhost, bind_port, "/status/motor", FORWARD);
  } else if ( M5.BtnA.wasReleased() ) {
    Serial.println("wasReleased");
    OscWiFi.send(localhost, bind_port, "/status/motor", REVERSE);
  }

  if ( M5.BtnB.wasReleased() ) {
    Serial.println("wasReleased");
    OscWiFi.send(localhost, bind_port, "/status/motor", BREAK);
  }

  // ホームボタン長押し モータ停止,WiFi設定リセット 
  if ( M5.BtnA.wasReleasefor(5000) ) {
    Serial.println("wasReleasefor");
    OscWiFi.send(localhost, bind_port, "/status/motor", BREAK);
    wifiManager.resetSettings();
    ESP.restart();
  }

  OscWiFi.update();
}