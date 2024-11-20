#include <MFRC522.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ESP8266HTTPClient.h>

#define SS_PIN 5
#define RST_PIN 4
#define RELAY_PIN 16

MFRC522 rfid(SS_PIN, RST_PIN);

const char* ssid = "MotoE32";
const char* password = "mateowifi01";

const int server_port = 8000;
const char* server_ip = "192.168.233.200";

const char* websocket_server = "192.168.233.200";
const int websocket_port = 8080;
// Modificamos la ruta del WebSocket para incluir el protocolo completo
const char* websocket_path = "/app/n0yio342pjj34u5ua96t";

WebSocketsClient webSocket;
StaticJsonDocument<200> doc;
StaticJsonDocument<200> innerDoc;

// Variable para control de reconexión
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000; // 5 segundos

unsigned long lastUnlockTime = 0;
const unsigned long UNLOCK_COOLDOWN = 8000;


void unlockDoor() {
    Serial.println("Abriendo cerradura...");
    digitalWrite(RELAY_PIN, LOW);
    delay(8000);
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Cerradura cerrada");
}

String getUID() {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    return uid;
}

bool sendRequest(String uid) {
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi no conectado!");
        return false;
    }

    HTTPClient http;
    WiFiClient client;
    
    String url = "http://" + String(server_ip) + ":" + String(server_port) + "/api/activation-records";
    Serial.println("Enviando petición a: " + url);
    
    http.begin(client, url);
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer 12|z08SU3fi4shF7UeeRSVoOCZN9XVbhsJa6LE9C7iEfaffc656");

    StaticJsonDocument<200> requestBody;
    requestBody["uid"] = uid;
    String requestBodyString;
    serializeJson(requestBody, requestBodyString);

    int statusResponseCode = http.POST(requestBodyString);
    Serial.println("Código de respuesta HTTP: " + String(statusResponseCode));

    bool shouldUnlock = false;

    if (statusResponseCode > 0) {
        String response = http.getString();
        Serial.println("Respuesta raw: " + response);

        StaticJsonDocument<300> jsonResponse;
        DeserializationError error = deserializeJson(jsonResponse, response);

        if (error) {
            Serial.println("Error parsing JSON");
        } else {
            shouldUnlock = jsonResponse["should_unlock"] | false;
        }
    } else {
        Serial.println("Error en la petición HTTP: " + http.errorToString(statusResponseCode));
    }

    http.end();

    return shouldUnlock;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Desconectado!");
            break;
            
        case WStype_CONNECTED: {
          
            Serial.println("[WSc] Conectado al servidor!");
            // Enviar mensaje de suscripción con el formato correcto de Laravel Reverb
            String subscribeMessage = "{\"event\":\"pusher:subscribe\",\"data\":{\"channel\":\"tag-rfid-read\"}}";
            webSocket.sendTXT(subscribeMessage);
            Serial.println("[WSc] Mensaje de suscripción enviado");
            break;
        }
            
        case WStype_TEXT: {
            Serial.printf("[WSc] Mensaje recibido: %s\n", payload);
            
            DeserializationError error = deserializeJson(doc, payload);
            if (!error) {
                const char* event = doc["event"];
                const char* channel = doc["channel"];
                const char* dataStr = doc["data"]; // Obtenemos el string de data
                
                Serial.printf("[WSc] Evento recibido: %s\n", event);
                
                // Verificar si es el evento de desbloqueo
                if (String(event) == "App\\Events\\UnlockEvent") {
                    // Parsear el objeto data que viene como string
                    DeserializationError innerError = deserializeJson(innerDoc, dataStr);
                    
                    if (!innerError) {
                        bool shouldUnlock = innerDoc["unlock"];
                        Serial.printf("Valor de unlock: %s\n", shouldUnlock ? "true" : "false");
                        
                        if (shouldUnlock) {
                            unlockDoor();
                        }
                    } else {
                        Serial.println("Error al parsear data del evento UnlockEvent");
                    }
                }
                // Mantener el manejo de eventos de Pusher
                else if (String(event) == "pusher:connection_established") {
                    Serial.println("[WSc] Conexión establecida con Pusher");
                }
                else if (String(event) == "pusher:subscription_succeeded") {
                    Serial.println("[WSc] Suscripción al canal exitosa");
                }
            } else {
                Serial.println("Error al parsear mensaje WebSocket");
            }
            break;
        }
            
        case WStype_ERROR:
          
            Serial.printf("[WSc] Error: %u\n", length);
            break;
            
        case WStype_PING:
            Serial.println("[WSc] Ping recibido");
            break;
            
        case WStype_PONG:
            Serial.println("[WSc] Pong recibido");
            break;
    }
}

void setupWebSocket() {
    Serial.println("[WSc] Iniciando conexión WebSocket...");
    // Configurar WebSocket con más detalles de debugging
    webSocket.begin(websocket_server, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    // Activar el heartbeat para mantener la conexión viva
    webSocket.enableHeartbeat(15000, 3000, 2);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }  // Esperar a que el puerto serial esté listo
    
    Serial.println("\nIniciando setup...");
    
    SPI.begin();
    rfid.PCD_Init();
    Serial.println("RFID inicializado");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Relay configurado");

    // Conexión WiFi con más información de debugging
    Serial.print("Conectando a WiFi");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi conectado!");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
    
    setupWebSocket();
    Serial.println("Setup completado!");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado. Reconectando...");
        WiFi.begin(ssid, password);
        delay(6000);
        return;
    }

    webSocket.loop();

    // Verificar si necesitamos reiniciar la conexión WebSocket
    if (!webSocket.isConnected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > reconnectInterval) {
            Serial.println("Intentando reconectar WebSocket...");
            setupWebSocket();
            lastReconnectAttempt = now;
        }
    }

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        
        unsigned long currentTime = millis();
        
        // Check if enough time has passed since last unlock
        if (currentTime - lastUnlockTime > UNLOCK_COOLDOWN) {
            String uid = getUID();
            Serial.println("UID leído: " + uid);
            
            bool shouldUnlock = sendRequest(uid);

            if(shouldUnlock) {
              lastUnlockTime = currentTime;
            }
            
            
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }

    rfid.PCD_Init();
    delay(50);  // Reducido el delay para mejor respuesta
}