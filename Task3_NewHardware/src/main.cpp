#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HCSR04.h>
#include <TM1637Display.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "ESPAsyncWebServer.h" 


#define LED_BUILTIN 48
#define D5 8
#define D3 6
#define A0 1
#define A1 2
#define D9 18
#define SDA 11
#define SCL 12

#define WLAN_SSID "Redmi"
#define WLAN_PASS "123456789"

WiFiClient client;

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void TaskDistanceMeasure(void *pvParameters);

//Define your components here
Adafruit_NeoPixel pixels3(4, D5, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);
UltraSonicDistanceSensor distance(10, 17);
AsyncWebServer server(80);
int CLK0P = 18;
int DIO = 21;
TM1637Display display = TM1637Display(CLK0P, DIO);


void MQTT_connect() {
  int8_t ret;

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
}

// Create a structure to hold all sensor data
struct SensorData {
  float temperature;
  float humidity;
  int moisture;
  int light;
  int distance;
  int fanspeed;
};

// Function to get all sensor data at once
String getSensorDataJson() {
  SensorData data;
  
  // Get temperature and humidity
  dht20.read();
  data.temperature = dht20.getTemperature();
  data.humidity = dht20.getHumidity();
  
  // Get moisture (from A0 pin)
  data.moisture = analogRead(A0);
  
  // Get light (from A1 pin)
  data.light = analogRead(A1);
  
  // Get distance
  data.distance = distance.measureDistanceCm(dht20.getTemperature());
  
  // Map Fan Speed
  data.fanspeed = map(ledcRead(0), 0, 255, 0, 100);

  // Create JSON string
  String jsonString = "{";
  jsonString += "\"temperature\":" + String(data.temperature) + ",";
  jsonString += "\"humidity\":" + String(data.humidity) + ",";
  jsonString += "\"moisture\":" + String(data.moisture) + ",";
  jsonString += "\"light\":" + String(data.light) + ",";
  jsonString += "\"distance\":" + String(data.distance) + ",";
  jsonString += "\"fanspeed\":" + String(data.fanspeed);
  jsonString += "}";
  
  return jsonString;
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
    :root {
        --temp: 0;
        --humi: 0;
        --mois: 0;
        --ligh: 0;
        --dist: 0;
    }
    
    body {
        background-color: #000;
        color: #fff;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
        font-family: Arial, Helvetica, sans-serif
    }
    .dashboard{
        display: flex;
        gap: 100px;
    }
    .gauge {
        display: flex;
        flex-direction: column;
        align-items: center;
        font-weight: bold;
    }

    .gauge .circle {
        position: relative;
        width: 150px;
        height: 150px;
        background: #333;
        border-radius: 50%;
        display: flex;
        justify-content: center;
        align-items: center;
    }

    .gauge .circle .value {
        position: absolute;
        z-index: 2;
        font-size: 24px;
        text-align: center;
        line-height: 1.2;
    }

    .gauge .circle::before {
        content: "";
        position: absolute;
        width: 110px;
        height: 110px;
        background-color: #000;
        border-radius: 50%;
        z-index: 1;
    }

    .gauge .circle[data-type="temperature"] {
        background: conic-gradient(#82e572 calc(var(--temp) / 40 * 100%), #333 calc(var(--temp) / 40 * 100%));;
    }
    .gauge .circle[data-type="humidity"] {
        background: conic-gradient(#82e572 calc(var(--humi) * 100% / 100), #333 calc(var(--humi) * 100% / 100));
    }
    .gauge .circle[data-type="moisture"] {
        background: conic-gradient(#82e572 calc(var(--mois) * 100% / 2000), #333 calc(var(--mois) * 100% / 2000));
    }
    .gauge .circle[data-type="light"] {
        background: conic-gradient(#82e572 calc(var(--ligh) * 100% / 3000), #333 calc(var(--ligh) * 100% / 3000));
    }
    .gauge .circle[data-type="distance"] {
        background: conic-gradient(#82e572 calc(var(--dist) * 100% / 400), #333 calc(var(--dist) * 100% / 400));
    }

    .range-labels {
        display: flex;
        justify-content: space-between;
        width: 130px;
        font-size: 14px;
        margin-top: 10px;
    }
    .fan-control {
        margin-top: 20px;
        display: flex;
        flex-direction: column;
        align-items: center;
    }
    .fan-control input[type="range"] {
        width: 300px;
        margin-bottom: 10px;
    }
    .fan-speed-display {
        color: #fff;
        font-size: 18px;
    }
    </style>
</head>
<body>
    <div class="section dashboard">
        <div class="gauge">
            <div class="circle" data-type="temperature">
                <div class="value">--<br>&deg;C</div>
            </div>
            <div class="range-labels">
                <span>20</span>
                <span>40</span>
            </div>
            <div class="label">Temperature</div>
        </div>

        <div class="gauge">
            <div class="circle" data-type="humidity">
                <div class="value">--<br>%</div>
            </div>
            <div class="range-labels">
                <span>0%</span>
                <span>100%</span>
            </div>
            <div class="label">Humidity</div>
        </div>

        <div class="gauge">
            <div class="circle" data-type="moisture">
                <div class="value">--<br>Value</div>
            </div>
            <div class="range-labels">
                <span>0</span>
                <span>2000</span>
            </div>
            <div class="label">Moisture</div>
        </div>

        <div class="gauge">
            <div class="circle" data-type="light">
                <div class="value">--<br>Lux</div>
            </div>
            <div class="range-labels">
                <span>0</span>
                <span>3000</span>
            </div>
            <div class="label">Light</div>
        </div>

        <div class="gauge">
            <div class="circle" data-type="distance">
                <div class="value">--<br>cm</div>
            </div>
            <div class="range-labels">
                <span>0</span>
                <span>400</span>
            </div>
            <div class="label">Distance</div>
        </div>
        
        <div class="fan-control">
            <input type="range" id="fanSpeedSlider" min="0" max="100" value="0">
            <div class="fan-speed-display">Fan Speed: <span id="fanSpeedValue">0</span>%</div>
        </div>
    </div>
    
    <script>
    function updateSensorData() {
        fetch('/api/sensor-data')
            .then(response => response.json())
            .then(data => {
                document.documentElement.style.setProperty('--temp', data.temperature);
                document.querySelector(
                    '.dashboard .gauge .circle[data-type="temperature"] .value'
                ).innerHTML = `${data.temperature}<br>°C`;
                
                document.documentElement.style.setProperty('--humi', data.humidity);
                document.querySelector(
                    '.dashboard .gauge .circle[data-type="humidity"] .value'
                ).innerHTML = `${data.humidity}<br>%`;

                document.documentElement.style.setProperty('--mois', data.moisture);
                document.querySelector(
                    '.dashboard .gauge .circle[data-type="moisture"] .value'
                ).innerHTML = `${data.moisture}<br>Value`;

                document.documentElement.style.setProperty('--ligh', data.light);
                document.querySelector(
                    '.dashboard .gauge .circle[data-type="light"] .value'
                ).innerHTML = `${data.light}<br>Lux`;

                document.documentElement.style.setProperty('--dist', data.distance);
                document.querySelector(
                    '.dashboard .gauge .circle[data-type="distance"] .value'
                ).innerHTML = `${data.distance}<br>cm`;

                document.getElementById('fanSpeedValue').textContent = data.fanSpeed;
                document.getElementById('fanSpeedSlider').value = data.fanSpeed;
            })
            .catch(error => console.error('Error:', error));
    }

    document.getElementById('fanSpeedSlider').addEventListener('input', function() {
        const fanSpeed = this.value;
        fetch(`/api/set-fan-speed?speed=${fanSpeed}`)
            .then(response => response.text())
            .then(data => {
                document.getElementById('fanSpeedValue').textContent = fanSpeed;
            })
            .catch(error => console.error('Error:', error));
    });

    setInterval(updateSensorData, 1000);
    </script>
</body>
</html>
)rawliteral";

void setupWebServer() {
  // Main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // API endpoint for all sensor data
  server.on("/api/sensor-data", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getSensorDataJson());
  });

    server.on("/api/set-fan-speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("speed")) {
      int speed = request->getParam("speed")->value().toInt();
      
      // Map fan speed (0-100) to PWM duty cycle (0-255)
      int pwmValue = map(speed, 0, 100, 0, 255);
      
      // Set fan speed via PWM
      ledcWrite(0, pwmValue);
      
      request->send(200, "text/plain", "Fan speed set to " + String(speed) + "%");
    } else {
      request->send(400, "text/plain", "Speed parameter missing");
    }
  });

  // Start server
  server.begin();
}

void setup() {

  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200); 

  Wire.begin(SDA, SCL);
  dht20.begin();
  lcd.begin();
  pixels3.begin();


  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ledcSetup(0, 5000, 8);  // Channel 0, 5000 Hz, 8-bit resolution
  ledcAttachPin(D9, 0);  // Attach pin to channel 0
  ledcWrite(0, 0);

  setupWebServer(); 

  xTaskCreate( TaskBlink, "Task Blink" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndRelay, "Task Soild Relay" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskDistanceMeasure, "Task Distance Measure" ,2048  ,NULL  ,2 , NULL);
  
  //Now the task scheduler is automatically started.
  Serial.printf("Basic Multi Threading Arduino Example\n");
  
  
  
}
int pubCount = 0;
void loop() {

}



/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskBlink(void *pvParameters) {  
  pinMode(LED_BUILTIN, OUTPUT);
  

  while(1) {                          
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED ON
    Serial.println("LED ON");
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED OFF
    Serial.println("LED OFF");
    delay(1000);
  }
}


void TaskTemperatureHumidity(void *pvParameters) {  // This is a task.
  while(1) {                          
    Serial.println("Task Temperature and Humidity");
    dht20.read();
    Serial.println(dht20.getTemperature());
    Serial.println(dht20.getHumidity());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20.getTemperature());
    lcd.setCursor(0, 1);
    lcd.print(dht20.getHumidity());

    delay(5000);
  }
}



void TaskDistanceMeasure(void *pvParameters) {
  while (1) {
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Distance Measure");
    int distance_cm = distance.measureDistanceCm(dht20.getTemperature());
    Serial.print(distance_cm);
    Serial.println(" cm");
    display.clear();
    display.setBrightness(7);
    display.showNumberDec(distance_cm);
    vTaskDelay(5000);
  }
  
}

void TaskSoilMoistureAndRelay(void *pvParameters) {  // This is a task.
  pinMode(D3, OUTPUT);

  while(1) {                          
    Serial.println("Task Soild and Relay");
    Serial.println(analogRead(A0));

    
    if(analogRead(A0) > 500){
      digitalWrite(D3, LOW);
    }
    if(analogRead(A0) < 50){
      digitalWrite(D3, HIGH);
    }
    delay(5000);
  }
}


void TaskLightAndLED(void *pvParameters) {  // This is a task.


  while(1) {                          
    Serial.println("Task Light and LED");
    Serial.println(analogRead(A1));

    if(analogRead(A1) < 350){
      pixels3.setPixelColor(0, pixels3.Color(255,0,0));
      pixels3.setPixelColor(1, pixels3.Color(255,0,0));
      pixels3.setPixelColor(2, pixels3.Color(255,0,0));
      pixels3.setPixelColor(3, pixels3.Color(255,0,0));
      pixels3.show();

    }
    if(analogRead(A1) > 550){
      pixels3.setPixelColor(0, pixels3.Color(0,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,0));
      pixels3.setPixelColor(2, pixels3.Color(0,0,0));
      pixels3.setPixelColor(3, pixels3.Color(0,0,0));
      pixels3.show();

    }
    delay(5000);
  }
}

/*//*/