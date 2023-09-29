#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>

#define PIN 2
#define PIN_R4 4
#define PIN_R3 25
#define PIN_R2 26
#define PIN_R1 27
#define PIN_RESET 21
#define SCK  (18)
#define MISO (19)
#define MOSI (23)
#define SS   (5)

Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(104, PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "esp";
const char* password = "Karolina2137";
const IPAddress staticIP(192, 168, 2, 20);
const IPAddress gateway(192, 168, 2, 1);
const IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP Restart</title>
</head>
<body>
    <button onclick="restartESP()">Restart ESP</button>
    <script>
        function restartESP() {
            fetch('/restart');
        }
    </script>
</body>
</html>
)rawliteral";



uint8_t key_a[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int led;
unsigned long startTime;
bool dimm = true;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_R4, OUTPUT);
  pinMode(PIN_R3, OUTPUT);
  pinMode(PIN_R2, OUTPUT);
  pinMode(PIN_R1, OUTPUT);
  digitalWrite(PIN_R4, 1);
  digitalWrite(PIN_R3, 1);
  digitalWrite(PIN_R2, 1);
  digitalWrite(PIN_R1, 1);
  pixels.begin();

  startTime = millis();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Łączenie z WiFi...");
  }

  WiFi.config(staticIP, gateway, subnet);

  server.on("/restart", HTTP_GET, handleRestart);
  server.on("/", HTTP_GET, handleRoot);

  server.begin();

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Nie znaleziono modułu PN53x");
    while (1);
  }

  nfc.SAMConfig();
  Serial.println("Czekam na kartę RFID/NFC...");

}

void loop() {
  if(rfid() == 0x1){
    handleRelay(PIN_R4);
    handleRelay(PIN_R3);
    handleRelay(PIN_R2);
    handleRelay(PIN_R1);
    handleLEDs();
  }
  for(int i = 0; i < 4; i++){
    if (millis() - startTime < 2000) {
      if(rfid() == 0x1){
        handleLEDs();
      }
    } else {
      startTime = millis();
      i--;
    }
  }
  while(1){
    dimm = false;
    handleLEDs();
  }
}

void handleRelay(uint8_t pin) {
  for(led=0; led <=104; led++) {
    setColor(led,0,0,0,0);
  }
  digitalWrite(pin, 0);
  digitalWrite(pin, 1);
  delay(100);
  digitalWrite(pin, 0);
  delay(100);
  digitalWrite(pin, 1);
  delay(250);
  digitalWrite(pin, 0);
  delay(250);
  digitalWrite(pin, 1);
  delay(400);
  digitalWrite(pin, 0);
  delay(1000);
  digitalWrite(pin, 1);
  delay(1000);
}

void handleLEDs() {
    for(led=0; led <=104; led++) {
      setColor(led,0,0,255,1);
    }
    for(led=0; led <=104; led++) {
      setColor(led,0,50,255,0);
    }
    for(led=0; led <=104; led++) {
      setColor(led,0,100,255,1);
    }
    for(led=0; led <=104; led++) {
      setColor(led,0,150,255,0);
    }
    for(led=0; led <=104; led++) {
      setColor(led,0,200,255,1);
    }
    for(led=0; led <=104; led++) {
      setColor(led,0,255,255,0);
    }
}

void setColor(int led, int redValue, int greenValue, int blueValue, int delayValue) {
  pixels.setPixelColor(led, pixels.Color(redValue, greenValue, blueValue)); 
  pixels.show();
  delay(delayValue);
}

int rfid() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  if(dimm = true){
    for(led=0; led <=104; led++) {
      setColor(led,0,0,0,1);
    }
  }

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    uint8_t data[1];
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, key_a)) {
      if (nfc.mifareclassic_ReadDataBlock(4, data)) {
        return data[0];
      } else {
        Serial.println("Błąd odczytu danych.");
      }
    } else {
      Serial.println("Błąd uwierzytelniania.");
    }
  }
  return 0;
}


void get_request(const char* responseText) {
    server.on("/status", HTTP_GET, [responseText](AsyncWebServerRequest *request){
        request->send(200, "text/plain", responseText);
    });
}

void handleRestart(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
}

void handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", INDEX_HTML);
}


