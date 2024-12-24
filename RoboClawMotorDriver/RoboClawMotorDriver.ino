// NodeMCU Motor Driver Control via Web Interface
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include<LiquidCrystal_I2C.h>
#include<Wire.h>
// Motor driver pins
#define ENA D7  // Speed control for Motor A
#define IN1 D8  // Direction control for Motor A
#define IN2 D3  // Direction control for Motor A
#define ENB D4  // Speed control for Motor B
#define IN3 D5  // Direction control for Motor B
#define IN4 D6  // Direction control for Motor B
LiquidCrystal_I2C lcd(0x27, 16, 2);
// WiFi access point credentials
const char *ssid = "NodeMCU_AP";
const char *password = "12345678";

// Web server instance
ESP8266WebServer server(80);
//lcd.clear();
  
// HTML webpage content
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Motor Control</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; background-color: #f4f4f9; }
    h1 { color: #333; }
    button { padding: 10px 20px; margin: 10px; font-size: 16px; }
  </style>
</head>
<body>
  <h1>Motor Driver Control</h1>
  <button onclick="sendCommand('FORWARD')">Forward</button>
  <button onclick="sendCommand('BACKWARD')">Backward</button><br>
  <button onclick="sendCommand('LEFT')">Left</button>
  <button onclick="sendCommand('RIGHT')">Right</button><br>
  <button onclick="sendCommand('STOP')">Stop</button><br>
  <label for="speed">Speed:</label>
  <input type="range" id="speed" min="0" max="1023" oninput="updateSpeed(this.value)"><br>
  <script>
    function sendCommand(command) {
      fetch(`/${command}`).then(response => console.log(response.status));
    }
    function updateSpeed(value) {
      fetch(`/SPEED?value=${value}`).then(response => console.log(response.status));
    }
  </script>
</body>
</html>
)=====";

// Function to set motor direction
void setMotorDirection(bool A1, bool A2) {
  digitalWrite(IN1, A1);
  digitalWrite(IN2, A2);
  digitalWrite(IN3, A1);
  digitalWrite(IN4, A2);
}

// Function to handle web commands
void handleRoot() {
  server.send(200, "text/html", MAIN_page);
}

void handleForward() {
  setMotorDirection(HIGH, LOW);
  server.send(200, "text/plain", "Moving Forward");
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Move Forward");
}

void handleBackward() {
  setMotorDirection(LOW, HIGH);
  server.send(200, "text/plain", "Moving Backward");
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Move Back");
}

void handleLeft() {
  setMotorDirection(LOW, HIGH);
  server.send(200, "text/plain", "Turning Left");
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Turning Left");
}

void handleRight() {
  setMotorDirection(HIGH, LOW);
  server.send(200, "text/plain", "Turning Right");
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Turning Right");
}

void handleStop() {
  setMotorDirection(LOW, LOW);
  server.send(200, "text/plain", "Stopping");
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Stopping");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    int speed = server.arg("value").toInt();
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    server.send(200, "text/plain", "Speed set to " + String(speed));
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void setup() {
  // Initialize pins
  lcd.begin();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(1,0);
  lcd.print("Hello All!");
  delay(1500);
  lcd.setCursor(0,1);
  lcd.print("Let's Ride!");
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Start WiFi access point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.begin(115200);
  Serial.print("AP IP Address: ");
  Serial.println(IP);

  // Define web server routes
  server.on("/", handleRoot);
  server.on("/FORWARD", handleForward);
  server.on("/BACKWARD", handleBackward);
  server.on("/LEFT", handleLeft);
  server.on("/RIGHT", handleRight);
  server.on("/STOP", handleStop);
  server.on("/SPEED", handleSpeed);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
