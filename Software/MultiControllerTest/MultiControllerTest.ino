#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define ESP32VER 3 //enter major version of ESP32 arduino board

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WLAN credentials
const char* ssid = "LV426";
const char* password = "19263854466404343353";

// Pin definitions for Molex Fan
const int MOLEX_PWM_PIN = 1;     // GPIO1 for PWM
const int MOLEX_RPM_PIN = 2;     // GPIO2 for RPM signal
const int AUX_EN_PIN = 38;       // GPIO38 for switch
const int MOLEX_EN_PIN = 8;

// Pin definitions for 0-10V Fan
const int CH1_PWM_PIN = 9;    // GPIO9 for Channel1 Output
const int CH1_DC_EN_PIN = 11; // GPIO11 to enable DC mode
const int CH2_PWM_PIN = 12;    // GPIO9 for Channel1 Output
const int CH2_DC_EN_PIN = 10; // GPIO11 to enable DC mode
const int CH3_PWM_PIN = 14;    // GPIO9 for Channel1 Output
const int CH3_DC_EN_PIN = 47; // GPIO11 to enable DC mode
const int CH4_PWM_PIN = 48;    // GPIO9 for Channel1 Output
const int CH4_DC_EN_PIN = 21; // GPIO11 to enable DC mode

// PWM configuration for Molex Fan
const int MOLEX_PWM_FREQ = 12000;  // 25 kHz
const int MOLEX_PWM_CHANNEL = 0;
const int MOLEX_PWM_RESOLUTION = 8;

// PWM configuration for 0-10V Fan
const int CH1_PWM_FREQ = 5000;   // 5 kHz
const int CH1_PWM_CHAN = 1;
const int CH1_PWM_RES = 8;

WebServer server(80);

// Global variables
volatile unsigned long lastPulseTime = 0;
volatile unsigned int pulseCount = 0;
unsigned int rpm = 0;
int molexFanSpeed = 0;           // Current speed for Molex fan (0-100%)
float analogFanVoltage = 6.5;    // Current voltage for 0-10V fan

// Interrupt Service Routine for RPM measurement
void IRAM_ATTR rpmISR() {
  pulseCount++;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // IP address
  display.setCursor(0,0);
  display.print("IP: ");
  display.println(WiFi.localIP());
  
  // Molex fan speed
  display.setCursor(0,16);
  display.print("Molex: ");
  display.print(molexFanSpeed);
  display.println("%");
  
  // Analog fan voltage
  display.setCursor(0,32);
  display.print("0-10V: ");
  display.print(analogFanVoltage, 1);
  display.println("V");
  
  // RPM
  display.setCursor(0,48);
  display.print(rpm);
  display.println(" RPM");
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize OLED
  Wire.begin(6, 5);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  Serial.println("Starting...");
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Starting...");
  display.display();
  
  // Initialize EEPROM
  EEPROM.begin(512);
  
  // Load saved values
  molexFanSpeed = EEPROM.read(0);
  if(molexFanSpeed > 100) molexFanSpeed = 50;
  
  analogFanVoltage = EEPROM.readFloat(4);
  if(analogFanVoltage < 0 || analogFanVoltage > 10) analogFanVoltage = 6.5;
  
  // Configure Aux Pin
  pinMode(AUX_EN_PIN, OUTPUT);
  digitalWrite(AUX_EN_PIN, LOW);

  // Configure Molex fan pins
  pinMode(MOLEX_RPM_PIN, INPUT_PULLUP);
  pinMode(MOLEX_EN_PIN, OUTPUT);
  digitalWrite(MOLEX_EN_PIN, LOW);
  
  // Configure 0-10V fan pins
  pinMode(CH1_DC_EN_PIN, OUTPUT);
  pinMode(CH1_PWM_PIN, OUTPUT);
  pinMode(CH2_DC_EN_PIN, OUTPUT);
  pinMode(CH2_PWM_PIN, OUTPUT);
  pinMode(CH3_DC_EN_PIN, OUTPUT);
  pinMode(CH3_PWM_PIN, OUTPUT);
  pinMode(CH4_DC_EN_PIN, OUTPUT);
  pinMode(CH4_PWM_PIN, OUTPUT);

  
  // Configure PWM channels
  #if ESP32VER < 3
    ledcSetup(MOLEX_PWM_CHANNEL, MOLEX_PWM_FREQ, MOLEX_PWM_RESOLUTION);
    ledcAttachPin(MOLEX_PWM_PIN, MOLEX_PWM_CHANNEL);

    ledcSetup(CH1_PWM_CHAN, CH1_PWM_FREQ, CH1_PWM_RES);
    ledcAttachPin(CH1_PWM_PIN, CH1_PWM_CHAN);
    ledcSetup(CH2_PWM_CHAN, CH1_PWM_FREQ, CH1_PWM_RES);
    ledcAttachPin(CH2_PWM_PIN, CH1_PWM_CHAN);
    ledcSetup(CH3_PWM_CHAN, CH1_PWM_FREQ, CH1_PWM_RES);
    ledcAttachPin(CH3_PWM_PIN, CH1_PWM_CHAN);
    ledcSetup(CH4_PWM_CHAN, CH1_PWM_FREQ, CH1_PWM_RES);
    ledcAttachPin(CH4_PWM_PIN, CH1_PWM_CHAN);
  #else
    //analogWriteFrequency(MOLEX_PWM_PIN,MOLEX_PWM_FREQ);
    if (ledcAttach(MOLEX_PWM_PIN,MOLEX_PWM_FREQ,MOLEX_PWM_RESOLUTION)){
      Serial.println("Sucessfully attached Molex PWM");
    }
    if (ledcAttach(CH1_PWM_PIN,CH1_PWM_FREQ,CH1_PWM_RES))
      Serial.println("Sucessfully attached CH1 PWM");
    if (ledcAttach(CH2_PWM_PIN,CH1_PWM_FREQ,CH1_PWM_RES))
      Serial.println("Sucessfully attached CH2 PWM");
    if (ledcAttach(CH3_PWM_PIN,CH1_PWM_FREQ,CH1_PWM_RES))
      Serial.println("Sucessfully attached CH3 PWM");
    if (ledcAttach(CH4_PWM_PIN,CH1_PWM_FREQ,CH1_PWM_RES))
      Serial.println("Sucessfully attached CH4 PWM");
    
    
    
    Serial.print("Molex Fan Freq: ");
    Serial.println(ledcReadFreq(MOLEX_PWM_PIN));
    Serial.print("ledc clock source: "); 
  attachInterrupt(digitalPinToInterrupt(MOLEX_RPM_PIN), rpmISR, FALLING);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting WiFi...");
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Define web server routes
  server.on("/", handleRoot);
  server.on("/setMolexSpeed", handleSetMolexSpeed);
  server.on("/setAnalogVoltage", handleSetAnalogVoltage);
  server.on("/getData", handleGetData);
  server.on("/switch", handleSwitch);
  server.begin();
  
  updateDisplay();
}

void loop() {
  server.handleClient();
  
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long lastRpmCalc = 0;
  
  // Calculate RPM every 500ms
  if(millis() - lastRpmCalc >= 500) {
    rpm = (pulseCount * 60);
    pulseCount = 0;
    lastRpmCalc = millis();
  }
  
  // Update display every 1000ms
  if(millis() - lastDisplayUpdate >= 1000) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}

//set all Voltages to the same value for now
void setAnalogFanVoltage(float voltage) {
  if (voltage <= 0.0) {
    ledcWrite(CH1_PWM_PIN, 0);
    ledcWrite(CH2_PWM_PIN, 0);
    ledcWrite(CH3_PWM_PIN, 0);
    ledcWrite(CH4_PWM_PIN, 0);
  } else {
    digitalWrite(CH1_DC_EN_PIN, HIGH); //analog mode
    digitalWrite(CH2_DC_EN_PIN, HIGH); //analog mode
    digitalWrite(CH3_DC_EN_PIN, HIGH); //analog mode
    digitalWrite(CH4_DC_EN_PIN, HIGH); //analog mode

    int dutyCycle = (voltage * 255) / 10.0;
    ledcWrite(CH1_PWM_PIN, dutyCycle);
    ledcWrite(CH2_PWM_PIN, dutyCycle);
    ledcWrite(CH3_PWM_PIN, dutyCycle);
    ledcWrite(CH4_PWM_PIN, dutyCycle);

  }
}

void handleRoot() {
  String html = "<!DOCTYPE html>\n"
                "<html>\n"
                "<head>\n"
                    "<title>Fan Control System</title>\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                    "<style>\n"
                        "body { font-family: Arial; text-align: center; margin: 20px; }\n"
                        ".slider { width: 80%; max-width: 400px; }\n"
                        ".control-group { margin: 30px 0; }\n"
                        "button { padding: 10px 20px; margin: 5px; }\n"
                    "</style>\n"
                "</head>\n"
                "<body>\n"
                    "<h1>Fan Control System</h1>\n"
                    "<div class=\"control-group\">\n"
                        "<h2>Molex Fan Control</h2>\n"
                        "<input type=\"range\" min=\"0\" max=\"100\" value=\"" + String(molexFanSpeed) + "\" class=\"slider\" id=\"molexSlider\">\n"
                        "<p>Speed: <span id=\"molexValue\">" + String(molexFanSpeed) + "</span>%</p>\n"
                        "<p>RPM: <span id=\"rpmValue\">" + String(rpm) + "</span></p>\n"
                        "<p>\n"
                            "<button onclick=\"switchGPIO(1)\">GPIO 38 ON</button>\n"
                            "<button onclick=\"switchGPIO(0)\">GPIO 38 OFF</button>\n"
                        "</p>\n"
                    "</div>\n"
                    "<div class=\"control-group\">\n"
                        "<h2>0-10V Control</h2>\n"
                        "<input type=\"range\" min=\"0\" max=\"100\" value=\"" + String(analogFanVoltage * 10) + "\" class=\"slider\" id=\"analogSlider\">\n"
                        "<p>Voltage: <span id=\"analogValue\">" + String(analogFanVoltage) + "</span>V</p>\n"
                    "</div>\n"
                    "<script>\n"
                        "var molexSlider = document.getElementById('molexSlider');\n"
                        "var analogSlider = document.getElementById('analogSlider');\n"
                        "var molexValue = document.getElementById('molexValue');\n"
                        "var analogValue = document.getElementById('analogValue');\n"
                        "var rpmValue = document.getElementById('rpmValue');\n"
                        
                        "molexSlider.oninput = function() {\n"
                            "molexValue.innerHTML = this.value;\n"
                            "fetch('/setMolexSpeed?speed=' + this.value);\n"
                        "};\n"
                        
                        "analogSlider.oninput = function() {\n"
                            "let voltage = (this.value / 10).toFixed(1);\n"
                            "analogValue.innerHTML = voltage;\n"
                            "fetch('/setAnalogVoltage?voltage=' + voltage);\n"
                        "};\n"

                        
                        "function switchGPIO(state) {\n"
                            "fetch('/switch?state=' + state);\n"
                        "}\n"
                        
                        "setInterval(function() {\n"
                            "fetch('/getData')\n"
                            ".then(response => response.json())\n"
                            ".then(data => {\n"
                                "rpmValue.innerHTML = data.rpm;\n"
                                "molexValue.innerHTML = data.molexSpeed;\n"
                                "analogValue.innerHTML = data.analogVoltage;\n"
                                "molexSlider.value = data.molexSpeed;\n"
                                "analogSlider.value = data.analogVoltage * 10;\n"
                            "});\n"
                        "}, 500);\n"
                    "</script>\n"
                "</body>\n"
                "</html>\n";
  server.send(200, "text/html", html);
}

void handleSetMolexSpeed() {
  static bool wasOn=true;
  Serial.print("Set Fan Speed: ");
  
  if(server.hasArg("speed")) {
    molexFanSpeed = server.arg("speed").toInt();
    molexFanSpeed = constrain(molexFanSpeed, 0, 100);
    Serial.println(molexFanSpeed);

    if(molexFanSpeed == 0) {
      ledcWrite(MOLEX_PWM_PIN, 0);
      //pinMode(MOLEX_EN_PIN, INPUT); //set to High-Z
      Serial.println("Fan OFF");
      digitalWrite(MOLEX_EN_PIN, LOW);
    } else {
      digitalWrite(MOLEX_EN_PIN, HIGH);
      //ledcWrite(MOLEX_PWM_PIN, map(molexFanSpeed, 0, 100, 255, 0));
      analogWrite(MOLEX_PWM_PIN, map(molexFanSpeed, 0, 100, 255, 0));
    }
    
    EEPROM.write(0, molexFanSpeed);
    EEPROM.commit();
    
    server.send(200, "text/plain", "OK");
  }
}

void handleSetAnalogVoltage() {
  if(server.hasArg("voltage")) {
    analogFanVoltage = server.arg("voltage").toFloat();
    analogFanVoltage = constrain(analogFanVoltage, 0, 10);
    
    setAnalogFanVoltage(analogFanVoltage);
    
    EEPROM.writeFloat(4, analogFanVoltage);
    EEPROM.commit();
    
    server.send(200, "text/plain", "OK");
  }
}

void handleSwitch() {
  if(server.hasArg("state")) {
    if(server.arg("state") == "1") {
      digitalWrite(AUX_EN_PIN, HIGH);
    } else {
      digitalWrite(AUX_EN_PIN, LOW);
    }
    server.send(200, "text/plain", "OK");
  }
}

void handleGetData() {
  String json = "{\"molexSpeed\":" + String(molexFanSpeed) + 
                ",\"analogVoltage\":" + String(analogFanVoltage) + 
                ",\"rpm\":" + String(rpm) + "}";
  server.send(200, "application/json", json);
}
