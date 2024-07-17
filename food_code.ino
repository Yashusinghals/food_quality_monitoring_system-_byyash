#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <LiquidCrystal.h>

// WiFi credentials
const char* ssid = "OppoA54";
const char* password = "01230123";

// ThingSpeak Channel settings
unsigned long channelID = 2526226;
const char* writeAPIKey = "NB2CM1GK3H2CNLST";

WiFiClient client;

// DHT Sensor setup
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ2 Sensor setup
const int MQ2Pin = A0;

// LCD Pin setup
LiquidCrystal lcd(D1, D2, D5, D6, D7, D8); // Ensure these are correct GPIOs

// Calibration factors
const float tempCalibrationFactor = 0.2; // Adjust this value as needed
const float humidityCalibrationFactor = -19; // Adjust this value as needed
const float mq2CalibrationFactor = 100; // Adjust this value as needed

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  dht.begin();

  connectToWiFi(); // Improved WiFi connection
  ThingSpeak.begin(client);
}

void loop() {
  // Read sensor values
  float humidity = dht.readHumidity() + humidityCalibrationFactor;
  float tempC = dht.readTemperature() + tempCalibrationFactor;
  int mq2Value = analogRead(MQ2Pin) + mq2CalibrationFactor;

  // Validate sensor readings
  bool dhtError = isnan(humidity) || isnan(tempC);
  if (dhtError && mq2Value < 0) { // Check if both sensors have issues
    Serial.println("Failed to read from both sensors!");
    lcd.clear();
    lcd.print("Both Sensors Error!");
  } else {
    String foodQuality;
    if (dhtError) {
      Serial.println("Failed to read from DHT sensor!");
      lcd.clear();
      lcd.print("DHT Error!");
      foodQuality = getFoodQualityMQ2Only(mq2Value);
      updateDisplayMQ2Only(mq2Value, foodQuality);
    } else if (mq2Value < 0) { // Assuming a negative value indicates an error in reading MQ2
      Serial.println("Failed to read from MQ2 sensor!");
      lcd.clear();
      lcd.print("MQ2 Error!");
      foodQuality = getFoodQualityDHTOnly(tempC, humidity);
      updateDisplayDHTOnly(tempC, humidity, foodQuality);
    } else {
      foodQuality = getFoodQuality(tempC, humidity, mq2Value);
      updateDisplay(tempC, humidity, mq2Value, foodQuality);
    }
    sendDataToThingSpeak(tempC, humidity, mq2Value);
    Serial.println("Food Quality: " + foodQuality);
  }

  delay(20000); // Update every 20 seconds
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void updateDisplay(float tempC, float humidity, int mq2Value, String foodQuality) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Food Quality:");
  lcd.setCursor(0, 1);
  lcd.print(foodQuality);
}

void updateDisplayMQ2Only(int mq2Value, String foodQuality) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQ2: ");
  lcd.print(mq2Value);
  lcd.setCursor(0, 1);
  lcd.print(foodQuality);
}

void updateDisplayDHTOnly(float tempC, float humidity, String foodQuality) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tempC);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.setCursor(0, 2);
  lcd.print(foodQuality);
}

void sendDataToThingSpeak(float tempC, float humidity, int mq2Value) {
  ThingSpeak.setField(1, tempC);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, mq2Value);
  if (ThingSpeak.writeFields(channelID, writeAPIKey) == 200) {
    Serial.println("Data sent successfully");
    lcd.setCursor(0, 3);
    lcd.print("Data sent OK");
  } else {
    Serial.println("Error sending data");
    lcd.setCursor(0, 3);
    lcd.print("Error sending data");
  }
}

String getFoodQuality(float tempC, float humidity, int mq2Value) {
  String tempQuality;

  if (tempC < 13) {
    tempQuality = "Very Bad";
  } else if (tempC < 20) {
    tempQuality = "Bad";
  } else if (tempC < 22) {
    tempQuality = "Moderate";
  } else {
    tempQuality = "Good";
  }

  String humidityQuality;
  if (humidity > 80) {
    humidityQuality = "Very Bad";
  } else if (humidity >= 70) {
    humidityQuality = "Bad";
  } else if (humidity >= 68) {
    humidityQuality = "Moderate";
  } else {
    humidityQuality = "Good";
  }

  String mq2Quality;
  if (mq2Value > 400) {
    mq2Quality = "Very Bad";
  } else if (mq2Value >=300) {
    mq2Quality = "Bad";
  } else if (mq2Value >= 280) {
    mq2Quality = "Moderate";
  } else {
    mq2Quality = "Good";
  }

  if (tempQuality == "Good" && humidityQuality == "Good" && mq2Quality == "Good") {
    return "Food is Good";
  } else if (tempQuality == "Very Bad" || tempQuality == "Bad" || humidityQuality == "Bad" || mq2Quality == "Bad") {
    return "Food is Bad";
  } else if (tempQuality == "Moderate" || humidityQuality == "Moderate" || mq2Quality == "Moderate") {
    return "Food is Moderate";
  } else {
    return "Food is Very Bad";
  }
}

String getFoodQualityMQ2Only(int mq2Value) {
  String mq2Quality;
  if (mq2Value > 700) {
    mq2Quality = "Very Bad";
  } else if (mq2Value > 500) {
    mq2Quality = "Bad";
  } else if (mq2Value >= 250) {
    mq2Quality = "Moderate";
  } else {
    mq2Quality = "Good";
  }
  return "Food is " + mq2Quality;
}

String getFoodQualityDHTOnly(float tempC, float humidity) {
  String tempQuality;
  if (tempC < 13) {
    tempQuality = "Very Bad";
  } else if (tempC < 26) {
    tempQuality = "Bad";
  } else if (tempC < 22) {
    tempQuality = "Moderate";
  } else {
    tempQuality = "Good";
  }

  String humidityQuality;
  if (humidity > 80) {
    humidityQuality = "Very Bad";
  } else if (humidity >= 70) {
    humidityQuality = "Bad";
  } else if (humidity >= 68) {
    humidityQuality = "Moderate";
  } else {
    humidityQuality = "Good";
  }

  if (tempQuality == "Good" && humidityQuality == "Good") {
    return "Food is Good";
  } else if (tempQuality == "Very Bad" || tempQuality == "Bad" || humidityQuality == "Bad") {
    return "Food is Bad";
  } else {
    return "Food is Moderate";
  }
}