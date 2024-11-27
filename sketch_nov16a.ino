#include <MFRC522.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Debug Logging Macro
#define DEBUG_PRINT(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)

// Hardware Pinout Configuration
#define SS_PIN 5
#define RST_PIN 4
#define RELAY_PIN 16

// WiFi Credentials
const char* WIFI_SSID = "MotoE32";
const char* WIFI_PASSWORD = "mateowifi01";

// Server Configuration
const char* API_HOST = "api.smartunlocker.site";
const char* WS_PATH = "/app/jbwlrcnx3u1hz1qglfyj";
const int WS_PORT = 443;

// Timing Constants
const unsigned long WIFI_RECONNECT_INTERVAL = 30000;
const unsigned long UNLOCK_COOLDOWN = 7000;
const unsigned long WS_RECONNECT_INTERVAL = 5000;
const unsigned long CARD_DEBOUNCE_DELAY = 1000;

class SmartUnlocker {
private:
    MFRC522 rfid;
    WebSocketsClient webSocket;
    
    struct SystemState {
        bool wifiConnected = false;
        bool wsConnected = false;
        unsigned long lastWiFiReconnect = 0;
        unsigned long lastCardRead = 0;
    } state;

    StaticJsonDocument<512> jsonBuffer;  

    void setupWiFi() {
        DEBUG_PRINT("[WIFI] Initializing WiFi Connection...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Detailed WiFi connection logging
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            DEBUG_PRINTF("[WIFI] Connecting... Attempt %d\n", ++attempts);
            if (attempts > 20) {
                DEBUG_PRINT("[WIFI] Connection Failed! Restarting...");
                ESP.restart();
            }
        }
        
        DEBUG_PRINTF("[WIFI] Connected! IP Address: %s\n", WiFi.localIP().toString().c_str());
    }

    void reconnectWebSocket() {
        DEBUG_PRINT("[WEBSOCKET] Attempting to reconnect...");
        
        webSocket.disconnect();
        webSocket.beginSSL(API_HOST, WS_PORT, WS_PATH);
        webSocket.setReconnectInterval(WS_RECONNECT_INTERVAL); // Reconectar cada 5s si falla
        webSocket.enableHeartbeat(15000, 3000, 3);  
        webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
            logWebSocketEvent(type, payload, length);
            handleWebSocketEvent(type, payload, length);
        });
    }

    void logWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
        switch(type) {
            case WStype_CONNECTED:
                DEBUG_PRINTF("[WEBSOCKET] Connected to %s\n", API_HOST);
                break;
            case WStype_DISCONNECTED:
                DEBUG_PRINT("[WEBSOCKET] Disconnected!");
                break;
            case WStype_TEXT:
                DEBUG_PRINTF("[WEBSOCKET] Received Text: %s\n", (char*)payload);
                break;
            case WStype_ERROR:
                DEBUG_PRINT("[WEBSOCKET] Error occurred!");
                break;
        }
    }

    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
        switch(type) {
            case WStype_CONNECTED:
                state.wsConnected = true;
                subscribeToChannel();
                break;
            
            case WStype_DISCONNECTED:
                state.wsConnected = false;
                DEBUG_PRINT("[WEBSOCKET] Connection lost. Will retry.");
                break;
            
            case WStype_TEXT:
                processWebSocketMessage(payload, length);
                break;
        }
    }

    void subscribeToChannel() {
        const char* subscribeMsg = "{\"event\":\"pusher:subscribe\",\"data\":{\"channel\":\"tag-rfid-read\"}}";
        DEBUG_PRINT("[WEBSOCKET] Subscribing to channel...");
        webSocket.sendTXT(subscribeMsg);
    }

    void processWebSocketMessage(uint8_t* payload, size_t length) {
        DeserializationError error = deserializeJson(jsonBuffer, payload);
        if (error) {
            DEBUG_PRINTF("[JSON] Parsing failed: %s\n", error.c_str());
            return;
        }

        const char* event = jsonBuffer["event"];
        DEBUG_PRINTF("[WEBSOCKET] Received Event: %s\n", event);

        if (strcmp(event, "App\\Events\\UnlockEvent") == 0) {
            unlockDoor();
        } 


    }

    bool sendActivationRequest(const String& uid) {
        DEBUG_PRINTF("[HTTP] Sending activation request for UID: %s\n", uid.c_str());
        
        WiFiClientSecure client;
        HTTPClient http;
        
        reconnectWebSocket();

        client.setInsecure();
        http.begin(client, "https://" + String(API_HOST) + "/api/activation-records");
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer ");

        String payload = "{\"uid\":\"" + uid + "\"}";
        int httpCode = http.POST(payload);
        
        

        DEBUG_PRINTF("[HTTP] Response Code: %d\n", httpCode);
        
        bool shouldUnlock = false;
        if (httpCode == 201) {
            String response = http.getString();
            DEBUG_PRINTF("[HTTP] Response: %s\n", response.c_str());
            
            DeserializationError error = deserializeJson(jsonBuffer, response);
            if (!error) {
                shouldUnlock = jsonBuffer["should_unlock"] | false;
                DEBUG_PRINTF("[HTTP] Unlock Decision: %s\n", shouldUnlock ? "UNLOCK" : "DENY");
            } else {
                DEBUG_PRINTF("[JSON] Parsing error: %s\n", error.c_str());
            }
        }

        http.end();
        return shouldUnlock;
    }

    void unlockDoor() {
        DEBUG_PRINT("[RELAY] Unlocking door...");
        digitalWrite(RELAY_PIN, LOW);
        delay(UNLOCK_COOLDOWN);
        digitalWrite(RELAY_PIN, HIGH);
        DEBUG_PRINT("[RELAY] Door locked.");
        delay(50); 
        rfid.PCD_Init();
    }

    String getCardUID() {
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
            uid += String(rfid.uid.uidByte[i], HEX);
        }
        return uid;
    }

public:
    SmartUnlocker() : rfid(SS_PIN, RST_PIN) {}

    void begin() {
        Serial.begin(115200);
        while(!Serial) { ; }  // Wait for Serial to be ready
        
        DEBUG_PRINT("\n[SYSTEM] Smart Unlocker Initializing...");
        
        SPI.begin();
        rfid.PCD_Init();
        DEBUG_PRINT("[RFID] RFID Module Initialized");
        
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, HIGH);
        DEBUG_PRINT("[RELAY] Relay Configured");
        
        setupWiFi();
        reconnectWebSocket();
        
        DEBUG_PRINT("[SYSTEM] Initialization Complete!");
    }

    void update() {
        webSocket.loop();
        
        // RFID Card Reading Logic
        if (rfid.PICC_IsNewCardPresent() && 
            rfid.PICC_ReadCardSerial() && 
            (millis() - state.lastCardRead > CARD_DEBOUNCE_DELAY)) {
            
            String uid = getCardUID();
            DEBUG_PRINTF("[RFID] Card Read. UID: %s\n", uid.c_str());
            
            if (sendActivationRequest(uid)) {
                unlockDoor();
            }
            
            rfid.PICC_HaltA();
            rfid.PCD_StopCrypto1();

            
            state.lastCardRead = millis();
        }
        
    }
};

SmartUnlocker unlocker;

void setup() {
    unlocker.begin();
}

void loop() {
    delay(50);
    unlocker.update();
    yield();
}