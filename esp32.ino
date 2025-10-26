/*
 * ESP32 Web Server for 4-Channel Relay and IR Control
 * * Hardware from your diagram:
 * - ESP32
 * - 8-Channel Relay Module (using 4 channels)
 * - IR Receiver
 * * Wiring:
 * - Relay IN1 -> GPIO 23
 * - Relay IN2 -> GPIO 22
 * - Relay IN3 -> GPIO 21
 * - Relay IN4 -> GPIO 19
 * - IR Recv 'S' -> GPIO 4
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRremote.h>

// --- WiFi Credentials ---
// Change these to your WiFi network
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Pin Definitions ---
// Relays (as seen in your diagram)
#define RELAY_PIN_1 23  // Light 1
#define RELAY_PIN_2 22  // Light 2
#define RELAY_PIN_3 21  // Fan 1
#define RELAY_PIN_4 19  // Fan 2

// IR Receiver
#define IR_RECV_PIN 4

// Array to hold the state of our 4 relays
// 0 = OFF, 1 = ON
bool relayStates[4] = { false, false, false, false };

// --- IR Remote Codes ---
// You MUST change these to match your remote!
// Use the "IRrecvDump" example from the IRremote library to find your codes.
#define IR_CODE_LIGHT_1_TOGGLE  0xFF30CF // Example: "1" button
#define IR_CODE_LIGHT_2_TOGGLE  0xFF18E7 // Example: "2" button
#define IR_CODE_FAN_1_TOGGLE    0xFF7A85 // Example: "3" button
#define IR_CODE_FAN_2_TOGGLE    0xFF10EF // Example: "4" button

// --- Global Objects ---
AsyncWebServer server(80); // Web server on port 80
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

// --- HTML Page (Stored in Program Memory) ---
// This is the index.html file that the ESP32 will send to the browser.
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Home Control</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background-color: #f0f2f5; color: #1c1e21;
            margin: 0; padding: 20px;
        }
        header {
            display: flex; justify-content: space-between; align-items: center;
            margin-bottom: 25px;
        }
        header h1 { margin: 0; font-size: 1.8rem; }
        .device-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
            gap: 20px;
        }
        .device-card {
            background-color: #ffffff;
            border-radius: 12px;
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
            padding: 20px;
            text-align: center;
        }
        .device-card h3 { margin-top: 0; margin-bottom: 15px; }
        .controls { display: flex; gap: 10px; }
        .controls button {
            border: none; border-radius: 8px; padding: 10px 0;
            flex-grow: 1; font-size: 1rem; font-weight: 700;
            cursor: pointer; transition: background-color 0.2s ease, color 0.2s ease;
        }
        .on-btn { background-color: #e7f3ff; color: #0866ff; }
        .off-btn { background-color: #fdecec; color: #d93025; }
        
        /* Active state for buttons */
        .on-btn.active { background-color: #0866ff; color: white; }
        .off-btn.active { background-color: #d93025; color: white; }
    </style>
</head>
<body>
    <header>
        <h1>My Home</h1>
        <div id="status-indicator">Connecting...</div>
    </header>
    <main>
        <div class="device-grid">
            <article class="device-card" data-device-id="0">
                <h3>💡 Light 1</h3>
                <div class="controls">
                    <button class="on-btn">On</button>
                    <button class="off-btn">Off</button>
                </div>
            </article>
            <article class="device-card" data-device-id="1">
                <h3>💡 Light 2</h3>
                <div class="controls">
                    <button class="on-btn">On</button>
                    <button class="off-btn">Off</button>
                </div>
            </article>
            <article class="device-card" data-device-id="2">
                <h3>🌀 Ceiling Fan 1</h3>
                <div class="controls">
                    <button class="on-btn">On</button>
                    <button class="off-btn">Off</button>
                </div>
            </article>
            <article class="device-card" data-device-id="3">
                <h3>🌀 Ceiling Fan 2</h3>
                <div class="controls">
                    <button class="on-btn">On</button>
                    <button class="off-btn">Off</button>
                </div>
            </article>
        </div>
    </main>
    
    <script>
        const statusIndicator = document.getElementById('status-indicator');

        // Function to send command to ESP32
        function setRelayState(relayIndex, state) {
            const url = `/${state}?relay=${relayIndex}`;
            fetch(url)
                .then(response => response.text())
                .then(text => {
                    console.log(text);
                    updateStatus(); // Update UI immediately after command
                })
                .catch(err => {
                    console.error('Fetch error:', err);
                    statusIndicator.innerText = 'Connection Lost';
                });
        }

        // Function to get current status from ESP32 and update UI
        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    statusIndicator.innerText = 'Connected';
                    // data is an array like [false, true, false, true]
                    data.forEach((state, index) => {
                        const card = document.querySelector(`.device-card[data-device-id="${index}"]`);
                        if (card) {
                            const onBtn = card.querySelector('.on-btn');
                            const offBtn = card.querySelector('.off-btn');
                            
                            onBtn.classList.toggle('active', state === true);
                            offBtn.classList.toggle('active', state === false);
                        }
                    });
                })
                .catch(err => {
                    console.error('Status error:', err);
                    statusIndicator.innerText = 'Connection Lost';
                });
        }

        // Add click listeners to all buttons
        document.querySelector('.device-grid').addEventListener('click', (event) => {
            const btn = event.target.closest('button');
            if (!btn) return; // Didn't click a button
            
            const card = btn.closest('.device-card');
            const relayIndex = card.dataset.deviceId;
            
            if (btn.classList.contains('on-btn')) {
                setRelayState(relayIndex, 'on');
            } else if (btn.classList.contains('off-btn')) {
                setRelayState(relayIndex, 'off');
            }
        });

        // Initial status update when page loads
        document.addEventListener('DOMContentLoaded', () => {
            updateStatus();
            // Poll for status every 2 seconds (to catch IR remote changes)
            setInterval(updateStatus, 2000);
        });
    </script>
</body>
</html>
)rawliteral";

// --- Utility Functions ---

// Sets the relay state and updates the physical pin
void setRelay(int relayIndex, bool state) {
  if (relayIndex < 0 || relayIndex > 3) return;
  
  relayStates[relayIndex] = state;
  
  int pinToControl = 0;
  switch (relayIndex) {
    case 0: pinToControl = RELAY_PIN_1; break;
    case 1: pinToControl = RELAY_PIN_2; break;
    case 2: pinToControl = RELAY_PIN_3; break;
    case 3: pinToControl = RELAY_PIN_4; break;
  }
  
  // Note: Relay modules are often "Active LOW", meaning
  // LOW turns the relay ON and HIGH turns it OFF.
  // We'll assume Active LOW.
  if (state == true) { // Turn ON
    digitalWrite(pinToControl, LOW);
  } else { // Turn OFF
    digitalWrite(pinToControl, HIGH);
  }
}

// Toggles a relay's state
void toggleRelay(int relayIndex) {
  setRelay(relayIndex, !relayStates[relayIndex]);
}

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  
  // Set relay pins as outputs
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);
  pinMode(RELAY_PIN_4, OUTPUT);
  
  // Initialize all relays to OFF (HIGH for Active LOW)
  digitalWrite(RELAY_PIN_1, HIGH);
  digitalWrite(RELAY_PIN_2, HIGH);
  digitalWrite(RELAY_PIN_3, HIGH);
  digitalWrite(RELAY_PIN_4, HIGH);

  // Start IR receiver
  irrecv.enableIRIn();
  
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- Define Web Server Routes ---

  // Route for the root URL ("/")
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // Send the HTML page stored in PROGMEM
    request->send_P(200, "text/html", index_html);
  });

  // Route for turning ON a relay
  // e.g., /on?relay=0
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("relay")) {
      int relayIndex = request->getParam("relay")->value().toInt();
      setRelay(relayIndex, true);
      request->send(200, "text/plain", "Relay " + String(relayIndex) + " turned ON");
    } else {
      request->send(400, "text/plain", "Missing 'relay' parameter");
    }
  });

  // Route for turning OFF a relay
  // e.g., /off?relay=2
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("relay")) {
      int relayIndex = request->getParam("relay")->value().toInt();
      setRelay(relayIndex, false);
      request->send(200, "text/plain", "Relay " + String(relayIndex) + " turned OFF");
    } else {
      request->send(400, "text/plain", "Missing 'relay' parameter");
    }
  });

  // Route for getting the status of all relays
  // Returns a JSON array like: [false, true, false, false]
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String jsonResponse = "[";
    for (int i = 0; i < 4; i++) {
      jsonResponse += (relayStates[i] ? "true" : "false");
      if (i < 3) jsonResponse += ",";
    }
    jsonResponse += "]";
    request->send(200, "application/json", jsonResponse);
  });

  // Start the server
  server.begin();
}

// --- Main Loop ---
void loop() {
  // Check if an IR signal is received
  if (irrecv.decode(&results)) {
    Serial.print("IR Code Received: 0x");
    Serial.println(results.value, HEX);

    // Match the code to an action
    switch (results.value) {
      case IR_CODE_LIGHT_1_TOGGLE:
        toggleRelay(0);
        Serial.println("Toggling Light 1");
        break;
      case IR_CODE_LIGHT_2_TOGGLE:
        toggleRelay(1);
        Serial.println("Toggling Light 2");
        break;
      case IR_CODE_FAN_1_TOGGLE:
        toggleRelay(2);
        Serial.println("Toggling Fan 1");
        break;
      case IR_CODE_FAN_2_TOGGLE:
        toggleRelay(3);
        Serial.println("Toggling Fan 2");
        break;
      default:
        Serial.println("Unknown IR code");
    }
    
    irrecv.resume(); // Receive the next value
  }
}
