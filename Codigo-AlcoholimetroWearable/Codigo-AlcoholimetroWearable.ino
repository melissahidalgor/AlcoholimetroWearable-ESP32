#include <WiFi.h>
#include "esp_wpa2.h" 
#include "esp_wifi.h" 
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h> 
#include <ESPmDNS.h>

#include "myMAX30102.h"
#include "myMQ3.h"

// ------------ Pines -----------
//#define ESP32C3
#ifdef ESP32C3
const int pin_MQ3 = 0;
const int pin_SDA = 8;
const int pin_SCL = 9;
#else
const int pin_MQ = 34; // VP
const int pin_SDA = 25;
const int pin_SCL = 26;
#endif

// Credenciales de Wi-Fi
//#define UAA
#ifdef UAA
const char* ssid = "RI-UAA";
const char* id = "al262931";
const char* password = "Cewo3317";
#else
//const char* ssid = "Melissa Galaxy A30";
//const char* password = "isue7235";
const char* ssid = "WiFi Gamer";
const char* password = "12345678";
#endif

// El nombre pagina web
const char* host = "alcoholimetro"; // http://alcoholimetro.local

AsyncWebServer server(80);

void iniciarMedicionSensores(); 
volatile bool medicionEnCurso = false;
volatile float ultimaOxigenacion = 0.0;
volatile int ultimoPulso = 0;
volatile float ultimoAlcohol = 0.0;

//________________________________________________________________________________ setup()
void setup() {
  Serial.begin(115200);
  delay(10);
 // ___________________________________________________________ SPIFFS begin
  Serial.println("Verificando SPIFFS");
  if (!SPIFFS.begin()) {
    Serial.println("¡Error al montar SPIFFS!");
    //ESP.restart(); // Reinicia si SPIFFS falla
  }else Serial.println("SPIFFS montadas");
  // ___________________________________________________________ WiFi begin
#ifdef UAA
    Serial.println();
    Serial.print("Connecting to Enterprise Network: ");
    Serial.println(ssid);

    WiFi.disconnect(true); // Disconnect from any previous connection
    WiFi.mode(WIFI_STA);   // Set WiFi to station mode

    // --- WPA2 Enterprise configuration ---
    // These functions are from esp_wpa2.h and interact with the underlying IDF
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)id, strlen(id));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)id, strlen(id)); // Often identity and username are the same for PEAP
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));

    esp_wifi_sta_wpa2_ent_enable(); // Enable WPA2 Enterprise mode

    WiFi.begin(ssid); // Start connection to the SSID
#else
  WiFi.begin(ssid, password);
#endif  
  Serial.print("Conectando a Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("✅ ¡Conectado exitosamente!");
  Serial.print("📶 IP local asignada: ");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  // ___________________________________________________________ mDNS
  if (!MDNS.begin(host)) {
    Serial.println("Error configurando el respondedor mDNS.");
    while(1) { // Detener si no se puede iniciar mDNS
      delay(1000);
    }
  }
  Serial.print("mDNS iniciado. Puedes acceder desde http://");
  Serial.print(host);
  Serial.println(".local");

  initMQ3();
  initMAX30102();
  
  // ___________________________________________________________ Server begin (con AsyncWebServer)

  // Manejar el archivo raíz (index.html)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Manejar style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Manejar script.js
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "application/javascript");
  });

  // Ruta para INICIAR la medición
  server.on("/iniciar_medicion", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!medicionEnCurso) { // Solo iniciar si no hay una medición en curso
      medicionEnCurso = true;
      request->send(200, "application/json", "{\"status\":\"iniciando\"}");
    } else {
      request->send(200, "application/json", "{\"status\":\"en_curso\"}");
    }
  });

  // Manejar la ruta para obtener datos de los sensores
  // Ruta para OBTENER el ESTADO y los DATOS de la medición
  server.on("/obtener_datos", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<200> jsonDocument;

    if (medicionEnCurso) {
      jsonDocument["status"] = "midiendo";
    } else {
      jsonDocument["status"] = "completado";
      jsonDocument["oxigenacion"] = ultimaOxigenacion;
      jsonDocument["pulso"] = ultimoPulso;
      jsonDocument["alcohol"] = ultimoAlcohol;
    }

    String jsonString;
    serializeJson(jsonDocument, jsonString);
    request->send(200, "application/json", jsonString);
  });

  // Manejar rutas no encontradas
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "404: Not found");
  });

  // Manejar rutas no encontradas 
  server.onNotFound(notFound);

  server.begin();
  Serial.println("Servidor web asíncrono iniciado");
}

void iniciarMedicionSensores() {
  Serial.println("Medición iniciada...");
  ultimaOxigenacion = leerSpO2();
  ultimoPulso = leerPulso();
  ultimoAlcohol = leerAlcohol();
  medicionEnCurso = false;
  Serial.println("Mediciones terminadas");

}

//________________________________________________________________________________ loop()
void loop() {
  if(medicionEnCurso)
   iniciarMedicionSensores();
}

// Función para manejar rutas no encontradas
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "404: Not found");
}
