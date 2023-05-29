#include <LiquidCrystal.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>


#define POWER_OFF 0xFFA25D
#define BUTTON_0  0xFF6897
#define BUTTON_1  0xFF30CF
#define BUTTON_2  0xFF18E7
#define BUTTON_3  0xFF7A85

AsyncWebServer server(80);

IPAddress IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const char* ssid = "WEATHER_REPORT0";
const char* password = "8caractere";

const int 
d0 = 4,
d1 = 16,
d2 = 5,
d3 = 18,
d4 = 19,
d5 = 21,
d6 = 22,
d7 = 23;
const int rs = 15;
const int e = 8;
const int mic_pin = 2;
const int ir_pin = 34;
float dBs = 0.f;

IRrecv receiver(ir_pin);
decode_results results;

const int pr_signal = 35;
const int pr_power = 32;
int pr_value = 0;

LiquidCrystal lcd(22, 21, 5, 18, 23, 19);

const char index_php[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <p> HELLO </p>
  <p> 
    <t id="pr"> -- </t> <t> val </t>
  </p>
<script>
setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("pr").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/pr", true);
  xhttp.send();
}, 1000);
</script>
</html>)rawliteral";

void read_db()
{
  unsigned int min = 1 << 13;
  unsigned int max = 0;
  unsigned int sensorValue = 0;
  int count = 1000;
  while (count--) {
    sensorValue = analogRead(mic_pin);
    if (sensorValue < min) min = sensorValue;
    if (sensorValue > max) max = sensorValue;
    //      delay(1);
  }
  sensorValue = max - min;
  float voltage = sensorValue * (3.3 / 3071.0);
  Serial.print("min="); Serial.println(min);
  Serial.print("max="); Serial.println(max);
  Serial.print(" voltage="); Serial.println(voltage);
  dBs = 20 * log10(voltage / 0.000632);
}

void read_pr()
{
  digitalWrite(pr_power, HIGH);
  delay(10);
  pr_value = analogRead(pr_signal); 
  digitalWrite(pr_power, LOW);

  Serial.print("value: ");
  Serial.println(pr_value);
}

void setup()
{
  Serial.begin(115200); // serial setup
  // lcd setup
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("db: ");
  // server + wifi setup
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(IP, gateway, subnet);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_php);
  });
  server.on("/pr", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(pr_value).c_str());
  }); 
  server.begin();
  delay(2000);
  analogReadResolution(12);
  // ir setup
  receiver.enableIRIn();
  // pin setup
  pinMode(mic_pin, INPUT);
  pinMode(pr_power, OUTPUT);
  pinMode(pr_signal, INPUT);
}

void loop()
{
  read_db();
  lcd.setCursor(4, 0);
  lcd.print(dBs);
  read_pr();
  if (receiver.decode(&results)) {
        // Serial.println(receiver.decodedIRData.decodedRawData, HEX);
        Serial.println(results.value, HEX);
        switch (results.value) {
          case POWER_OFF:
            ESP.restart();
            break;
        }
        delay(50);
        receiver.resume();
  }
  
  delay(400);
}
