-- Table to define the devices connected to your ESP32
CREATE TABLE devices (
    id INT AUTO_INCREMENT PRIMARY KEY,
    device_name VARCHAR(100) NOT NULL,
    location VARCHAR(100),
    esp_id VARCHAR(50),      -- To identify which ESP32 (e.g., "living_room")
    relay_index INT NOT NULL -- The index (0-3) we used in the code
);

-- Table to log every on/off event
CREATE TABLE event_logs (
    log_id BIGINT AUTO_INCREMENT PRIMARY KEY,
    device_id INT NOT NULL,
    action VARCHAR(10) NOT NULL, -- 'ON' or 'OFF'
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    source VARCHAR(20) NOT NULL,  -- 'WEB' or 'IR'
    
    FOREIGN KEY (device_id) REFERENCES devices(id)
);

-- --- Example Data ---

-- Add your 4 devices from the circuit
INSERT INTO devices (device_name, location, esp_id, relay_index)
VALUES
    ('Light 1', 'Living Room', 'esp32_living', 0),
    ('Light 2', 'Living Room', 'esp32_living', 1),
    ('Ceiling Fan 1', 'Living Room', 'esp32_living', 2),
    ('Ceiling Fan 2', 'Living Room', 'esp32_living', 3);

-- Example of logging an event (your server would run this)
-- This would be run when the server receives a 'notify' call from the ESP32
INSERT INTO event_logs (device_id, action, source)
VALUES
    (1, 'ON', 'WEB'); -- User turned on 'Light 1' via the webpage

INSERT INTO event_logs (device_id, action, source)
VALUES
    (3, 'OFF', 'IR'); -- User turned off 'Ceiling Fan 1' via the IR remote
