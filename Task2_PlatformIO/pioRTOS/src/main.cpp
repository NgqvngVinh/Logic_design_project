#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define LED_BUILTIN 48
#define D5 8
#define D3 6
#define A0 1
#define A1 2
#define SDA 11
#define SCL 12

#define WLAN_SSID "abcde"
#define WLAN_PASS "12345678"
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "justacow224"
//#define AIO_KEY         "_OyRe257X6kZiinZUBGJswdDWKwkb" // Uncomment and add aio in the beginning (aio_...)

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);


/****************************** Feeds ***************************************/


// Setup a feed called 'tasktemperature_pub' for publishing.
Adafruit_MQTT_Publish tasktemperature_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/tasktemperature");

// Setup a feed called 'taskhumidity_pub' for publishing.
Adafruit_MQTT_Publish taskhumidity_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/taskhumidity");

// Setup a feed called 'tasksoilmoisture' for publishing.
Adafruit_MQTT_Publish tasksoilmoisture_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/tasksoilmoisture");

// Setup a feed called 'tasklight_pub' for publishing.
Adafruit_MQTT_Publish tasklight_pub = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/tasklight");

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);

//Define your components here
Adafruit_NeoPixel pixels3(4, D5, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);  // wait 10 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200); 

  dht20.begin(SDA, SCL);
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

  
  xTaskCreate( TaskBlink, "Task Blink" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndRelay, "Task Soild Relay" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  
  
  //Now the task scheduler is automatically started.
  Serial.printf("Basic Multi Threading Arduino Example\n");
  
  
  
}
int pubCount = 0;
void loop() {
  MQTT_connect();
  mqtt.processPackets(10000);
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}



/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskBlink(void *pvParameters) {  // This is a task.
  // initialize digital LED_BUILTIN on pin 13 as an output.
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
    tasktemperature_pub.publish(dht20.getTemperature());
    Serial.println(dht20.getHumidity());
    taskhumidity_pub.publish(dht20.getHumidity());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20.getTemperature());
    lcd.setCursor(0, 1);
    lcd.print(dht20.getHumidity());

    delay(5000);
  }
}

void TaskSoilMoistureAndRelay(void *pvParameters) {  // This is a task.
  pinMode(D3, OUTPUT);

  while(1) {                          
    Serial.println("Task Soild and Relay");
    Serial.println(analogRead(A0));
    tasksoilmoisture_pub.publish(uint32_t(analogRead(A0)));
    
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
    tasklight_pub.publish(uint32_t(analogRead(A1)));
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
