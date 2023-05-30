#include <LiquidCrystal.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include "dht.h"


#define BUTTON_0 0xFF6897
#define BUTTON_1 0xFF30CF
#define BUTTON_2 0xFF18E7
#define BUTTON_3 0xFF7A85
#define BUTTON_9 0xFF52AD

// Server + WiFi
AsyncWebServer server(80);
IPAddress IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const char* ssid = "WEATHER_REPORT0";
const char* password = "8caractere";

// LCD
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
LiquidCrystal lcd(22, 21, 5, 18, 23, 19);

// microphone
const int mic_pin = 2;
const int ir_pin = 34;
float dBs = 0.f;

// ir receiver
IRrecv receiver(ir_pin);
decode_results results;
enum Status {
  TEMP,
  HUM,
  PR
};
Status current = TEMP;

// water level
const int pr_signal = 35;
const int pr_power = 32;
float pr_value = 0.f;

// dht
const int dht_pin = 25;
float humidity = 0.f;
float temperature = 0.f;
DHT dht(dht_pin);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<style>
  body {
    text-align: center;
  }
  #reset {
    position: absolute;
    right: 0%;
    top: 0%;
  }
  #refresh {
    position: absolute;
    left: 0%;
    top: 0%;
  }
</style>
<html>
  <body>
    <button id="reset" onclick="reset_function()"> Reset </button>
    <button id="refresh" onclick="window.location.reload()"> Refresh </button>
    <title> WEATHER_REPORT0 </title>
    <p> -Temperature- </p>
    <p>
      <t id="temp"> %TEMP% </t> <t> &deg;C </t>
    </p>
    <p> -Humidity- </p>
    <p>
      <t id="hum"> %HUM% </t> <t> &percnt; </t>
    </p>
    <p> -Rainfall- </p>
    <p>
      <t id="pr"> %PR% </t> <t> mm </t>
    </p> 
  </body>
<script>
function reset_function(){
  let response = prompt('Are you sure? Type "reset" if you want to reset it. If not type anything else:');
  if (response === "reset") {
    let xhttp = new XMLHttpRequest();
    xhttp.open("POST", "/reset", true);
    xhttp.onreadystatechange = function(){
      alert("Reseting... It might take a while...");
    };
    xhttp.send();
  }
};
function PR() {
  let xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("pr").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/pr", true);
  xhttp.send();
}
function TEMP() {
  let xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temp").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temp", true);
  xhttp.send();
}
function HUM() {
  let xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("hum").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/hum", true);
  xhttp.send();
}
setInterval(PR(), 10000);
setInterval(TEMP(), 10000);
setInterval(HUM(), 10000);
</script>
</html>)rawliteral";

String processor(const String& var) {
  if(var == "TEMP"){
    return String(temperature);
  }
  else if(var == "HUM") {
    return String(humidity);
  } else if(var == "PR") {
    return String(pr_value);
  }
  return String();
}

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
  pr_value = float(analogRead(pr_signal)) / 100.f; 
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
  lcd.print("Temperature: ");
  // server + wifi setup
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(IP, gateway, subnet);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    auto response = request->beginResponse_P(200, "text/html", index_html, processor);
    response->addHeader("Refresh", "5");
    request->send(response);
  });
  server.on("/pr", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(pr_value).c_str());
  });
  server.on("/temp", HTTP_GET, [](AsyncWebServerRequest * request){
    request->send_P(200, "text/plain", String(temperature).c_str());
  });
  server.on("/hum", HTTP_GET, [](AsyncWebServerRequest * request){
    request->send_P(200, "text/plain", String(humidity).c_str());
  });
  server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "ok");
    ESP.restart();
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
  bool skip = false;
  read_db();
  read_pr();
  dht.read_dht();
  temperature = dht.temperature;
  humidity = dht.humidity;
  switch (current) {
    case TEMP:
      lcd.setCursor(0, 0);
      lcd.print("Temperature:");
      lcd.setCursor(0, 1);
      lcd.print(temperature);
      lcd.print("C       ");
      break;
    case HUM:
      lcd.setCursor(0, 0);
      lcd.print("Humidity:   ");
      lcd.setCursor(0, 1);
      lcd.print(humidity);
      lcd.print("%       ");
      break;
    case PR:
      lcd.setCursor(0, 0);
      lcd.print("Rainfall:   ");
      lcd.setCursor(0, 1);
      lcd.print(pr_value);
      lcd.print("mm      ");
      break;
  }
  if (receiver.decode(&results)) {
        Serial.println(results.value, HEX);
        switch (results.value) {
          case BUTTON_0:
            ESP.restart();
            break;
          case BUTTON_1:
            current = TEMP;
            break;
          case BUTTON_2:
            current = HUM;
            break;
          case BUTTON_3:
            current = PR;
            break;
          case BUTTON_9:
            skip = true;
            break;
        }
        delay(50);
        receiver.resume();
  }
  if (!skip)
    delay(300);
}
