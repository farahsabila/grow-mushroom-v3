/*************************************************************

  This is a code for controlling mushroom chamber environment
 *************************************************************/

/* Information from Blynk Device Info */
#define BLYNK_TEMPLATE_ID "******************"
#define BLYNK_TEMPLATE_NAME "Mushroom Chamber"
#define BLYNK_AUTH_TOKEN "*******************"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials
char ssid[] = "********";
char pass[] = "********";

BlynkTimer timer;

// Temperature and humidity sensors library, pins, and variables
#include <DHT.h>
DHT dht[] = {
  {15, DHT22},
  {4, DHT22},
  {5, DHT22},
};
double temp_set = 25;
double humidity_set = 90;

//Capacitive soil moisture variables
const int soilpin = A0;
const int AirValue = 2750;  
const int WaterValue = 1150;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

//Light intensity sensor library, pins, and variables
#include <BH1750.h>
#include <Wire.h>
BH1750 lightMeter;
uint16_t lux;

//Variables for Control LED Strip
int dutyCycle = 0;
/* Setting PWM Properties */
const int PWMFreq = 5000; /* 5 KHz */
const int PWMChannel = 0;
const int PWMResolution = 12;
const int MAX_DUTY_CYCLE = (int)(pow(2, PWMResolution) - 1);
const int ledStrip = 13;
int lux_set = 500;

//Declare actuators pins and variables
#define fan 25
#define ventilation_fan 12
#define humidifier 33
#define cooler1 14
#define cooler2 26
#define cooler3 27

//Variables for Timer/Millis.
unsigned long previousMillis = 0;
uint64_t interval = 5 * 60 * 1000; //--> Capture and Send a photo every 20 seconds.
int fan_setup = 1;

//Other variables needed
#define SDA 21
#define SCL 22

// This function is called every time the Virtual Pins state changes
BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  temp_set = param.asDouble();
  Serial.print("Temp Set: ");
  Serial.println(temp_set);
}

BLYNK_WRITE(V7)
{
  // Set incoming value from pin V7 to a variable
  humidity_set = param.asDouble();
  Serial.print("Humidity Set: ");
  Serial.println(humidity_set);
}

BLYNK_WRITE(V8)
{
  // Set incoming value from pin V8 to a variable
  lux_set = param.asInt();
  Serial.print("Light Set: ");
  Serial.println(lux_set);
}

void temp_control() {
  double temp[3];
  bool eventTrigger = false;
  for (int i = 0; i < 3; i++) {
    temp[i] = dht[i].readTemperature();
  }

  Serial.println("");
  Serial.print("Temp: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(temp[i]);
    Serial.print("|");
  }
  Serial.println("");

  // Update actual temperature value to pin v1, v2, and v3 in Blynk
  Blynk.virtualWrite(V1, temp[0]);
  Blynk.virtualWrite(V2, temp[1]);
  Blynk.virtualWrite(V3, temp[2]);

  double range1 = 0.5;
  double range2 = 0.5;

  if (temp[0] > temp_set + range1) {
    digitalWrite(cooler1, HIGH);
    digitalWrite(cooler2, HIGH);
  }
  else if (temp[0] <= temp_set - range1) {
    digitalWrite(cooler1, LOW);
    digitalWrite(cooler2, LOW);
  }

  if (temp[1] > temp_set + range2) {
    digitalWrite(cooler3, HIGH);
  }
  else if (temp[0] <= temp_set - range2) {
    digitalWrite(cooler3, LOW);
  }

  if (temp[0] > (temp_set + range2) && eventTrigger == false) {
    eventTrigger = true;
    //Serial.println(eventTrigger);
    Blynk.logEvent("temperature_alert", String("Temperature is ") + temp[0] + String(" °C now, greater than ") + temp_set + String(" °C. Please check the cooling system!"));
  }
  else if(temp[0] <= (temp_set + range2)){
    eventTrigger = false;
  }
}

void humidity_control() {
  double humid[3];
  bool eventTrigger = false;
  
  for (int i = 0; i < 3; i++) {
    humid[i] = dht[i].readHumidity();
  }

  Serial.println("");
  Serial.print("Humidity: ");
  for (int i = 0; i < 3; i++) {
    Serial.print(humid[i]);
    Serial.print("|");
  }

  // Update actual temperature value to pin V2 in Blynk
  Blynk.virtualWrite(V4, humid[0]);
  Blynk.virtualWrite(V5, humid[1]);
  Blynk.virtualWrite(V6, humid[2]);
  
  if (humid[0] < humidity_set) {
    digitalWrite(humidifier, HIGH);
    digitalWrite(fan, HIGH);
  }
  else {
    digitalWrite(humidifier, LOW);
    digitalWrite(fan, LOW);
  }

  if (humid[1] < humidity_set) {
    digitalWrite(humidifier, HIGH);
    digitalWrite(fan, HIGH);
  }
  else {
    digitalWrite(humidifier, LOW);
    digitalWrite(fan, LOW);
  }
  int range = 5;
  if (humid[0] < (humidity_set - range) && eventTrigger == false) {
    eventTrigger = true;
    //Serial.println(eventTrigger); 
    Blynk.logEvent("humidity_alert", String("Humidity is ") + humid[0] + String("% now, lower than ") + humidity_set + String("%. Please check the humidifier and water reservoir level!"));
  }
  else if(humid[0] >= (humidity_set - range)){
    eventTrigger = false;
  }
}

void soil_moisture() {
  soilMoistureValue = analogRead(soilpin);
  Serial.println(soilMoistureValue);
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
  if(soilmoisturepercent > 100){
    Serial.println("100 %");
  }
  else if(soilmoisturepercent <0){
    Serial.println("0 %");
  }
  else if(soilmoisturepercent >=0 && soilmoisturepercent <= 100){
    Serial.print(soilmoisturepercent);
    Serial.println("%");
  }

  Blynk.virtualWrite(V10, soilmoisturepercent);
}

void light_control() {
  bool eventTrigger = false;
  if (lightMeter.begin()) {
    lux = lightMeter.readLightLevel();
    //Serial.println("");
    Serial.print("Light(lx): ");
    Serial.println(lux);
  }
  else {
    Serial.println(F("Error initialising BH1750"));
  }

  Blynk.virtualWrite(V9, lux);

  while(lux < lux_set && dutyCycle <= MAX_DUTY_CYCLE){
    /* Increasing the LED brightness with PWM */
    dutyCycle++;
    ledcWrite(PWMChannel, dutyCycle);
    delay(3);
    //delayMicroseconds(100);
  }
  while(lux > lux_set && dutyCycle >= 0){
    /* Decreasing the LED brightness with PWM */
    dutyCycle--;
    ledcWrite(PWMChannel, dutyCycle);
    delay(3);
    //delayMicroseconds(100);
  }

  int range = 100;
  if (lux > (lux_set + range) && eventTrigger == false) {
    eventTrigger = true;  
    //Serial.println(eventTrigger); 
    Blynk.logEvent("light_alert", String("Light intensity is ") + lux + String(" lux now, greater than ") + lux_set + String("%. Please check the light system!"));
  }
  else if(lux <= (lux_set + range)){
    eventTrigger = false;
  }
}

void setup()
{
  // Debug console
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) {
    dht[i].begin();
  }

  pinMode(cooler1, OUTPUT);
  pinMode(cooler2, OUTPUT);
  pinMode(cooler3, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(humidifier, OUTPUT);
  pinMode(ventilation_fan, OUTPUT);

  ledcSetup(PWMChannel, PWMFreq, PWMResolution);
  ledcAttachPin(ledStrip, PWMChannel);

  digitalWrite(ventilation_fan, HIGH);
  Serial.print("Ventilation fan ON");

  Wire.begin(SDA, SCL);
  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
  lightMeter.begin();
  light_control();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // You can also specify server:
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);
  //Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, IPAddress(192,168,1,100), 8080);

  // Setup a function to be called every second
  //timer.setInterval(1500L, myTimerEvent);
  timer.setInterval(5000L, humidity_control);
  timer.setInterval(1000L, temp_control);
  timer.setInterval(5000L, light_control);
  timer.setInterval(1000L, soil_moisture);
}

void loop()
{
  Blynk.run();
  timer.run();

  if (fan_setup == 1) {
    interval = 5 * 60 * 1000;
  }
  else if (fan_setup == 0) {
    interval = 55 * 60 * 1000;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { 
    previousMillis = currentMillis;
    if (fan_setup == 1) {
      fan_setup = 0;
      digitalWrite(ventilation_fan, LOW);
      //Serial.println(fan_setup);
      Serial.println("Ventilation Fan OFF");
    }
    else if (fan_setup == 0) {
      fan_setup = 1;
      digitalWrite(ventilation_fan, HIGH);
      //Serial.println(fan_setup);
      Serial.println("Ventilation Fan ON");
    }
  }
}
