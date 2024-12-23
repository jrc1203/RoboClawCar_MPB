#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// Network credentials - Replace with your desired values
const char* ssid = "RoboClaw_AP";
const char* password = "12345678";

// Pin definitions
const int servoPin = D1;  // GPIO5

// IMPORTANT: Voltage considerations
// Currently configured for 3.3V testing from NodeMCU
// TODO: When switching to 5V external power:
// 1. Connect external 5V to servo power
// 2. Connect grounds together
// 3. Keep signal wire connected to NodeMCU D1

// Server instance
ESP8266WebServer server(80);

// Servo configuration
Servo clawServo;
int currentAngle = 0;  // Tracks current servo position
int targetAngle = 0;   // Target angle from web interface

// Servo movement parameters
const int updateInterval = 15;    // Milliseconds between position updates
const int smoothStep = 2;         // Degrees to move per step for smooth motion
unsigned long lastUpdate = 0;     // Timestamp of last position update

// HTML for the web interface
const char webPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>Robot Arm Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin: 0px auto; padding: 20px; }
        .slider-container { margin: 20px; }
        .slider { width: 80%; max-width: 400px; }
        #angleDisplay { font-size: 24px; margin: 20px; }
    </style>
</head>
<body>
    <h1>Robot Arm Control</h1>
    <div class="slider-container">
        <input type="range" min="0" max="180" value="0" class="slider" id="servoSlider">
    </div>
    <div id="angleDisplay">0°</div>
    <script>
        var slider = document.getElementById("servoSlider");
        var display = document.getElementById("angleDisplay");
        slider.oninput = function() {
            display.innerHTML = this.value + "°";
            fetch('/setAngle?value=' + this.value);
        }
        // Update current position every second
        setInterval(function() {
            fetch('/getCurrentAngle')
                .then(response => response.text())
                .then(angle => {
                    display.innerHTML = angle + "°";
                    slider.value = angle;
                });
        }, 1000);
    </script>
</body>
</html>
)=====";

void setup() {
    Serial.begin(115200);
    
    // Initialize servo
    clawServo.attach(servoPin);
    
    // Set servo to initial position
    clawServo.write(currentAngle);
    
    // Configure Access Point
    WiFi.softAP(ssid, password);
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    // Set up web server routes
    server.on("/", handleRoot);
    server.on("/setAngle", handleSetAngle);
    server.on("/getCurrentAngle", handleGetAngle);
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    updateServoPosition();
}

// Serve the web interface
void handleRoot() {
    server.send(200, "text/html", webPage);
}

// Handle angle updates from the slider
void handleSetAngle() {
    if (server.hasArg("value")) {
        targetAngle = server.arg("value").toInt();
        targetAngle = constrain(targetAngle, 0, 180);
        server.send(200, "text/plain", "OK");
    }
}

// Return current servo angle
void handleGetAngle() {
    server.send(200, "text/plain", String(currentAngle));
}

// Smooth servo movement implementation
void updateServoPosition() {
    if (millis() - lastUpdate >= updateInterval) {
        if (currentAngle != targetAngle) {
            // Move towards target angle smoothly
            if (currentAngle < targetAngle) {
                currentAngle += min(smoothStep, targetAngle - currentAngle);
            } else {
                currentAngle -= min(smoothStep, currentAngle - targetAngle);
            }
            clawServo.write(currentAngle);
        }
        lastUpdate = millis();
    }
}

/* Future Enhancements:
 * 1. Position Memory
 * To maintain servo position after reboot:
 * - Add EEPROM.h at the top
 * - In setup(), add: currentAngle = EEPROM.read(0);
 * - In handleSetAngle(), add: EEPROM.write(0, targetAngle);
 * 
 * 2. Motor Driver Integration
 * - Define L298N pins at the top
 * - Create motor control functions
 * - Add motor control endpoints to the web server
 * 
 * 3. Linear Actuator
 * - Define additional servo pin
 * - Create another Servo instance
 * - Extend web interface with additional slider
 */