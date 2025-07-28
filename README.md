# RoboClawCar_MPB

Hey there, fellow maker! 👋  
Welcome to **RoboClawCar_MPB** — a Wi-Fi-controlled, 4WD robotic car with a 3D-printed 4-DOF arm and claw, powered by an ESP32 and a slick, modern web interface.  
Ready to build, hack, and have some fun? Let’s dive in!

---

## 🚀 What Makes This Project Awesome?

- **4WD Powerhouse:**  
  Four BO DC motors, driven by an L298N, let you zip, spin, and cruise with style.

- **A Real Robotic Arm:**  
  4× MG90S servos (base, shoulder, elbow, gripper) — pick, place, wave, fist-bump!

- **Wi-Fi Freedom:**  
  ESP32 turns into a hotspot, so any phone or computer can control your bot — no apps, just your browser.

- **Gorgeous Web Control:**  
  Touch-friendly sliders, arrow buttons, dark mode toggle, record/playback motions — all from a slick interface.

- **Power Done Right:**  
  A beefy UBEC (5V, 3A+) keeps servos happy and resets at bay.

- **Hacker-Friendly:**  
  Clean code, easy pin changes, add sensors when you’re ready.

- **Bring Your Ideas:**  
  STL files for the arm are coming — remix, upgrade, show off!

---

## 🛠 What You’ll Need

| Part         | Details                                |
|--------------|----------------------------------------|
| Microcontroller | ESP32 (NodeMCU recommended)        |
| Motor Driver | L298N Dual H-Bridge                    |
| Motors       | 4× BO DC Gear Motors                   |
| Servos       | 4× MG90S (for the arm & gripper)       |
| Chassis      | 4WD robot frame                        |
| Battery      | 3× 3.7V Li-ion (total ~12V)            |
| UBEC         | 5V, 3A min (for servos!)               |
| Wiring       | Jumpers, terminals, patience           |

> _No sensors in this starter build. But that’s your future upgrade!_

---

## ⚡️ How to Hook It Up

**Power:**  
- 12V battery powers both L298N (motors) and UBEC (servos).
- ESP32 gets 5V, typically from the L298N’s 5V out — or use your own 5V supply.

**ESP32 Pin Map:**

| Function         | GPIO Pin |
|------------------|:-------:|
| Right ENA        | 32      |
| Right IN1        | 16      |
| Right IN2        | 17      |
| Left ENB         | 23      |
| Left IN3         | 18      |
| Left IN4         | 19      |
| Arm Base Servo   | 27      |
| Shoulder Servo   | 26      |
| Elbow Servo      | 25      |
| Gripper Servo    | 33      |

**Important:**  
Servos get their power from the UBEC — NOT the ESP32!

---

## 🖼 Connection Diagram

Check out this visual to avoid the “why won’t it move?!” blues:

![Connection Diagram](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/images/29865617/8ea62102-1b42-4c19-8575-626ff3de00be/Connection.jpg)

---

## 🧑‍💻 Software Setup (You Got This!)

1. **Download/clone this repo**.
2. **Fire up Arduino IDE** and open `Final_CODE_BLUE_THEME/Final_CODE_BLUE_THEME.ino`.
3. **Install these libraries:**  
   - `WiFi.h` (ESP32) / `ESP8266WiFi.h` (ESP8266)
   - `AsyncWebServer`
   - `AsyncTCP` (ESP32) / `ESPAsyncTCP` (ESP8266)
   - `ESP32Servo`
4. **Select Board:**  
   - For ESP32: “ESP32 Dev Module”
5. **Upload** and watch the magic happen.

---

## 🌐 Your Robot, Your Rules — From Any Browser

1. **Turn on the robot.**
2. **Connect** to Wi-Fi SSID:  
   - `MPB_RoboClawCar` / password: `12345678`
3. **Open your browser** to the IP shown in Serial Monitor (usually `192.168.4.1`).
4. **Control everything:**  
   - Drive with arrow buttons  
   - Move the arm and gripper  
   - Adjust speed  
   - Record and play back fancy moves  
   - Go dark mode if you’re feeling moody 😎

---

## 🦾 3D Printed Arm & Claw

STL files **coming soon!**  
Make it your own — remix and share back if you design something cooler!

---

## 💡 Pro Tips

- **Power:**  
  UBEC for servos is a must — brownouts are the enemy!
- **Motors not moving?**  
  Check your L298N wiring and battery.
- **Web UI not loading?**  
  Double-check your Wi-Fi and IP address.
- **Want to use ESP8266?**  
  Change the pin numbers in the code and update the libraries.

---

## 🛠️ Troubleshooting

- “Servos twitch or ESP32 resets!”  
  → Check UBEC and battery quality.
- “Motors dead, ESP32 fine.”  
  → L298N wiring, motor connections, or battery voltage.
- “Web interface unresponsive.”  
  → Browser/device on the right Wi-Fi? IP correct?
- Still stuck?  
  Open an [issue](https://github.com/jrc1203/RoboClawCar_MPB/issues) or share your setup!

---

## 🙏 Credits & License

- Code, web UI, and design by [@jrc1203](https://github.com/jrc1203)
- Built with love, coffee, and lots of trial-and-error
- MIT License — fork, remix, have fun!

---

**Ready to roll? Power it up, open your browser, and bring your robot to life!**  
And hey, I’d love to see your builds — tag or DM me your epic creations! 🚗🤖✋
# RoboClawCar

This is the repo for a 4wd robotic chassis car with 3d printed Claw and arm.
The Claw has a 3 DOF (degree of freedom).
Use:-
Final_CODE_BLUE_THEME - for code 

Microcontroller : Esp32 (recommended) or esp 8266 (you need to change the pin numbers accordingly for esp 8266)
After powering up ESP will create a access point(hotspot) which you have to connect with your mobile phone via wifi 
ssid and pass- in code 


# Imp point: 
For powering the servos of the claw use UBEC Module 5v 3amp or any Buck converter of minimum 3 amp

Connection: -
![Doc1_page-0001 (1)](https://github.com/user-attachments/assets/37640d5a-1a2a-4690-8a20-97e6403878ad)
