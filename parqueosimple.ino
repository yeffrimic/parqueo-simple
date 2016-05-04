
#include <Wire.h>
#include <HMC5883L.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7
int iPinTrigger = 5;
int iPinEcho = 4;
const char* ssid = "iPhone de Jorge";
const char* password = "chiviricuarta";
const char* mqtt_server = "67.228.191.108";
int estado;
long lastMsg = 0;
int promedio;
int degrees;
boolean magsensor1, a;
String parqueo;
long now, lastpub;
HMC5883L compass;
boolean ultrasonic1;
int comparative;
void magsensorsetup() {

  Wire.begin(D5, D4); //sda, scl
  // Initialize Initialize HMC5883L
  Serial.println("Initialize HMC5883L");
  while (!compass.begin())
  {
    Serial.println("Could not find a valid HMC5883L sensor, check wiring!");
    delay(500);
  }

  // Set measurement range
  compass.setRange(HMC5883L_RANGE_1_3GA);

  // Set measurement mode
  compass.setMeasurementMode(HMC5883L_CONTINOUS);

  // Set data rate
  compass.setDataRate(HMC5883L_DATARATE_75HZ);

  // Set number of samples averaged
  compass.setSamples(HMC5883L_SAMPLES_8);

  // Set calibration offset. See HMC5883L_calibration.ino
  compass.setOffset(0, 0);
}
WiFiClient espClient;
PubSubClient client(espClient);




void setup() {
  delay(30000);
  magsensorsetup();
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  for (int x = 0; x < 30; x++) {
    magsensor();
    promedio += degrees;
  }
  promedio /= 30;  //    checkTime ();
  degrees = 0;
  comparative = promedio ;

  while (!client.connected()) {
    reconnect();
  }
  publishAverage();
  lastpub = millis();
  now = millis();
}

void setup_wifi() {
  pinMode(iPinTrigger, OUTPUT);
  pinMode(iPinEcho, INPUT);
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void publishAverage() {

  StaticJsonBuffer<1024> jsonbuffer;
  JsonObject& root = jsonbuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& data = d.createNestedObject("data");
  data["average"] = promedio;
  char payload[1024];
  root.printTo(payload, sizeof(payload));

  Serial.print("Sending payload: ");
  Serial.println(payload);
  if (client.publish("outTopic", payload, byte(sizeof(payload)))) {
    Serial.println("Publish OK");
  }
  else {
    Serial.println("Publish FAILED");
  }
  delay(1000);

}

void publishData() {
  StaticJsonBuffer<1024> jsonbuffer;
  JsonObject& root = jsonbuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& data = d.createNestedObject("data");
  data["Mag"] = degrees;
  data["US"] = estado;
  data["state"] = parqueo;
  //   data["timestamp"] = ISO8601;
  char payload[1024];
  root.printTo(payload, sizeof(payload));

  Serial.print("Sending payload: ");
  Serial.println(payload);
  if (client.publish("outTopic", payload, byte(sizeof(payload)))) {
    Serial.println("Publish OK");
  }
  else {
    Serial.println("Publish FAILED");
  }
  delay(1000);

}
void loop() {
  if (!ultrasonic1) {
    ultrasonicsensor();
    if (estado < 50) {
      ultrasonic1 = true;
      now = millis();
    }
  }
  if (ultrasonic1) {
    magsensor();
    if (degrees < comparative - 10 || degrees > comparative + 10) {
      magsensor1 = true;
      parqueo = "ocupado";
    } else {
      ultrasonicsensor();
      if (estado > 50) {
        ultrasonic1 = false; 
        magsensor1 = false;
      parqueo = "desocupado";

      }
    }
    if (magsensor1) {
      Serial.println(now);
      Serial.println((millis() / 1000));
      Serial.println(millis());
     
    }
  }
 if (now - lastpub > 30000) {
        if (a != magsensor1) {
          publishData();
          a = magsensor1;
          lastpub = millis();
        }
      }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
now=millis();
}

void ultrasonicsensor() {

  digitalWrite(iPinTrigger, LOW);
  digitalWrite(iPinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(iPinTrigger, LOW);


  //Read the data from the sensor
  double fTime = pulseIn(iPinEcho, HIGH);
  //convert to seconds
  fTime = fTime / 1000000;
  double fSpeed = 347.867;
  //Get the distance in meters
  double fDistance = fSpeed * fTime;
  //Convert to centimeters;
  fDistance = fDistance * 100;
  //Get one way distance
  fDistance = fDistance / 2;

  Serial.print(fDistance);
  Serial.println("cm");
  estado = int(fDistance);
  delay(100);
}
void magsensor() {

  Vector norm = compass.readNormalize();

  // Calculate heading
  float heading = atan2(norm.YAxis, norm.XAxis);

  // Set declination angle on your location and fix heading
  // You can find your declination on: http://magnetic-declination.com/
  // (+) Positive or (-) for negative
  // For Bytom / Poland declination angle is 4'26E (positive)
  // Formula: (deg + (min / 60.0)) / (180 / M_PI);
  float declinationAngle = (1.0 + (20.0 / 60.0)) / (180 / M_PI);
  heading += declinationAngle;

  // Correct for heading < 0deg and heading > 360deg
  if (heading < 0)
  {
    heading += 2 * PI;
  }

  if (heading > 2 * PI)
  {
    heading -= 2 * PI;
  }

  // Convert to degrees
  float headingDegrees = heading * 180 / M_PI;

  // Output
  Serial.print(" Heading = ");
  Serial.print(heading);
  Serial.print(" Degress = ");
  Serial.print(headingDegrees);
  Serial.println();
  degrees = headingDegrees;

  delay(100);
}
