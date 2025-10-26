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
