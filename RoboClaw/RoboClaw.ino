#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// Wi-Fi credentials
const char* ssid = "RoboClaw_AP";     // SSID for Wi-Fi Access Point
const char* password = "12345678";  // Password for Access Point

ESP8266WebServer server(80); // Create a web server on port 80
Servo myServo;               // Servo object

int servoPin = D1; // GPIO2 (D4) for servo
int servoAngle = 90; // Initial servo angle

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Setting up Access Point...");

  // Start Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP Address: ");
  Serial.println(IP);

  // Attach the servo to the specified pin
  myServo.attach(servoPin);
  myServo.write(servoAngle); // Move servo to initial angle (90 degrees)

  // Define server routes
  server.on("/", handleRoot);                 // Main page
  server.on("/update", handleUpdateServo);    // Update servo position
  server.begin();                             // Start the server
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); // Handle incoming client requests
}

// Handle the root page (web interface)
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Robotic Arm Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
    </head>
    <body style="font-family: Arial; text-align: center; padding: 20px;">
      <h2>3D Printed Robotic Arm Controller</h2>
      <p>Servo Angle: <span id="servoAngle">90</span>°</p>
      <input type="range" min="0" max="180" value="90" id="slider" oninput="updateServo(this.value)">
      <br><br>
      <script>
        function updateServo(angle) {
          document.getElementById('servoAngle').innerText = angle;
          fetch('/update?angle=' + angle); // Send angle to server
        }
      </script>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html); // Send HTML content
}

// Handle servo angle updates
void handleUpdateServo() {
  if (server.hasArg("angle")) {
    servoAngle = server.arg("angle").toInt();
    if (servoAngle >= 0 && servoAngle <= 180) {
      myServo.write(servoAngle); // Move servo to the requested angle
      Serial.println("Servo Angle: " + String(servoAngle) + "°");
      server.send(200, "text/plain", "OK"); // Respond with "OK"
    } else {
      server.send(400, "text/plain", "Invalid angle"); // Respond with error
    }
  } else {
    server.send(400, "text/plain", "No angle provided"); // Respond with error
  }
}
