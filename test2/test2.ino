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
const int updateInterval = 10;    // Milliseconds between position updates
const int smoothStep = 2;         // Degrees to move per step for smooth motion
const int deadzone = 0;           // ±2 degrees deadzone to reduce noise 
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
        #desiredPosition { font-size: 18px; margin: 10px; }
        #angleDisplay { font-size: 24px; margin: 20px; display: flex; justify-content: space-between; }
        #barContainer {
            width: 80%;
            max-width: 400px;
            height: 20px;
            background: linear-gradient(to right, red, green);
            position: relative;
            margin: 20px auto;
            border: 1px solid black;
        }
        #indicator {
            width: 7px;
            height: 30px;
            background: black;
            position:   absolute;
            top: 0;
            transform: translateX(0%);
            transform: translateY(-16.6%);
        }
    </style>
</head>
<body>
    <h1>Robot Arm Control</h1>
    <div id="desiredPosition">Desired Position: 0&deg;</div>
    <div class="slider-container">
        <input type="range" min="0" max="180" value="0" class="slider" id="servoSlider">
    </div>
    <div id="angleDisplay">
        <span>Closing Claw</span>
        <span id="currentPosition">Current Position: 0&deg;</span>
        <span>Opening Claw</span>
    </div>
    <div id="barContainer">
        <div id="indicator" style="left: 0%;"></div>
    </div>
    <script>
        var slider = document.getElementById("servoSlider");
        var desiredPosition = document.getElementById("desiredPosition");
        var currentPosition = document.getElementById("currentPosition");
        var indicator = document.getElementById("indicator");

        slider.oninput = function() {
            desiredPosition.innerHTML = `Desired Position: ${this.value}&deg;`;
            fetch('/setAngle?value=' + this.value);
        };

        // Update current position and indicator every 100ms
        setInterval(function() {
            fetch('/getCurrentAngle')
                .then(response => response.text())
                .then(angle => {
                    currentPosition.innerHTML = `Current Position: ${angle}&deg;`;
                    let positionPercent = (angle / 180) * 100;
                    indicator.style.left = `${positionPercent}%`;
                });
        }, 100);
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
        if (abs(currentAngle - targetAngle) > deadzone) {
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
