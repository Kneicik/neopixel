#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>

// 0x1 wiatr  .23
// 0x2 ogien  .22
// 0x3 woda .21
// 0x4 ziemia .20

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
#define NUMPIXELS 104

Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(104, PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "esp";
const char* password = "Karolina2137";
const IPAddress staticIP(192, 168, 2, 21);
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
  if(rfid() == 0x3){
    handleRelay(PIN_R1, PIN_R2, PIN_R3, PIN_R4);
    handleLEDs();
    for(int i = 0; i < 4; i++){
      if (millis() - startTime < 2000) {
        if(rfid() == 0x3){
          handleLEDs();
        }
      } else {
        startTime = millis();
        i--;
      }
    }
  if(rfid() == 0x3){
    while(1){
      dimm = false;
      handleLEDs();
    }
  }
}
  }

void handleRelay(uint8_t pin, uint8_t pin2, uint8_t pin3, uint8_t pin4) {
  unsigned long startTime = millis();
  unsigned long elapsedTime = 0;

  while (elapsedTime < 2000) { // 2000 ms = 2 sekundy
    uint8_t selectedPin;
    uint8_t randPin = random(1, 5); // Losuje liczbę od 1 do 4

    switch (randPin) {
      case 1:
        selectedPin = pin;
        break;
      case 2:
        selectedPin = pin2;
        break;
      case 3:
        selectedPin = pin3;
        break;
      case 4:
        selectedPin = pin4;
        break;
    }

    digitalWrite(selectedPin, random(0, 2)); // Losowo ustawia stan wysoki (1) lub niski (0)

    delay(50); // Krótka przerwa między zmianami stanu pinów
    elapsedTime = millis() - startTime;
  }

  // Ustawia wszystkie piny w stanie wysokim na koniec
  digitalWrite(pin, HIGH);
  digitalWrite(pin2, HIGH);
  digitalWrite(pin3, HIGH);
  digitalWrite(pin4, HIGH);
}


void handleLEDs() {
    int led;
    int steps = 5; // Ilość kroków
    int incrementValue = 200 / steps; // Wartość o jaką zwiększamy czerwony kolor w każdym kroku

    for (int i = 0; i <= steps; i++) {
        int redValue = i * incrementValue;
        for (led = 0; led <= 104; led++) {
            setColor(led, redValue, 0, 255, i % 2);
        }
    }
}





void setColor(int led, int redValue, int greenValue, int blueValue, int delayValue) {
  pixels.setPixelColor(led, pixels.Color(redValue, greenValue, blueValue)); 
  pixels.show();
  delay(delayValue);
}

void setColorAll(uint8_t r, uint8_t g, uint8_t b) {
  for(int i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(r,g,b));
  }
  pixels.show();
}

int rfid() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  setColorAll(0,0,0);

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


