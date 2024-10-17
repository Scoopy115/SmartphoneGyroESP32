#include <Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

#define LED1_PIN 25  // GPIO pin for DEBUG LED
#define SERVO_1 26  // Servo 1 pin
#define SERVO_2 27  // Servo 2 pin

Servo servoRoll;
Servo servoPitch;

TaskHandle_t gyroRequest; // Create a taskhandle to retrieve gyro data on core 0 (this was done to try and make it work with as little delay as possible, but didn't do much in the end)

float alphaValue = 0.0;
float betaValue = 0.0;
float gammaValue = 0.0;

const char* ssid = ""; // Wifi SSID 
const char* password = ""; // WiFi Password
const char* serverAddress = ""; // Web address on which the php script is available
const int serverPort = 443; // Use 443 for HTTPS

AsyncWebServer server(80);
HTTPClient http;

// Boolean to check if an HTTP request is in progress
bool httpRequestInProgress = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);

  xTaskCreatePinnedToCore(
    gyroRequestCode, /* Function to implement the task */
    "gyroRequest", /* Name of the task */
    10000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    NULL,  /* Task handle. */
    0); /* Core where the task should run */

  // Attach servomotors
  servoRoll.attach(SERVO_1);
  servoPitch.attach(SERVO_2);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Route for fetching orientation data
  server.on("/scripts/read-orientation.php", HTTP_GET, [](AsyncWebServerRequest *request){
    // Check if an HTTP request is already in progress
    if (!httpRequestInProgress) {
      // Set the flag to indicate that an HTTP request is now in progress
      httpRequestInProgress = true;

      // Request orientation data from the PHP script
      httpGETRequest("/scripts/read-orientation.php", request);
    } else {
      // Send a 503 Service Unavailable response if an HTTP request is already in progress
      request->send(503, "text/plain", "HTTP request in progress. Please try again later.");
    }
  });

  // Start server
  server.begin();
  Serial.println("Setup completed.");
}

void httpGETRequest(const char* endpoint, AsyncWebServerRequest *request) {
  char urlBuffer[256];
  sprintf(urlBuffer, "%s%s", serverAddress, endpoint);

  // Start the request
  http.begin(urlBuffer);

  // Perform the GET request asynchronously
  int httpCode = http.GET();

  // Check for a successful request
  if (httpCode > 0) {
    digitalWrite(LED1_PIN, HIGH);
    // Parse the JSON response
    DynamicJsonDocument jsonDoc(512);
    deserializeJson(jsonDoc, http.getString());

    // Extract orientation data
    alphaValue = jsonDoc["alpha"].as<float>();
    betaValue = jsonDoc["beta"].as<float>();
    gammaValue = jsonDoc["gamma"].as<float>();

    // Process orientation data as needed
    Serial.printf("Alpha: %.2f | Beta: %.2f | Gamma: %.2f\n", alphaValue, betaValue, gammaValue);

    // Send the response if a request object is provided
    if (request != nullptr) {
      request->send(200, "application/json", http.getString());
    }
  } else {
    Serial.println("HTTP request failed");
  }

  // Keep connection open for next request
  digitalWrite(LED1_PIN, LOW);

  // Set the flag to indicate that the HTTP request is now complete
  httpRequestInProgress = false;
  Serial.printf("Free Heap Size: %d\n", ESP.getFreeHeap());
  delay(75);
}

void fetchOrientationData() {
  // Check if an HTTP request is not already in progress
  if (!httpRequestInProgress) {
    // Set the flag to indicate that an HTTP request is now in progress
    httpRequestInProgress = true;

    // Fetch orientation data from the server/database asynchronously
    httpGETRequest("/scripts/read-orientation.php", nullptr);
  }
}

//gyroRequestCode makes sure it requests the gyro information from the server on core0, while the rest of the code runs on core 1.
void gyroRequestCode( void * pvParameters ){
  while (1){ // While loop makes sure the fetchOrientationData function keeps looping indefinitely.
    fetchOrientationData();
  }
}

void loop() {
  rollOrientation(gammaValue);
  pitchOrientation(betaValue);
}

void rollOrientation(float gammaValue) {
  // Map value from the range -180 to 180 to servo range 0 to 180
  int servoRollPosition = map(gammaValue, -40, 40, 0, 180);
  servoRoll.write(servoRollPosition);
}

void pitchOrientation(float betaValue) {
  // Map value from the range -180 to 180 to servo range 0 to 180
  int servoPitchPosition = map(betaValue, -40, 40, 180, 0);
  servoPitch.write(servoPitchPosition);
}