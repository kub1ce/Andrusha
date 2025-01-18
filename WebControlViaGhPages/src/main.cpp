#include <Arduino.h>
#include <painlessMesh.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Frontend";
const char* password = "Backend;";
const char* ip = "192.168.222.11"; // Желаемый IP адрес ESP32 в вашей локальной сети

WebServer server(80);
WiFiClient wfc;

bool LED = false;


void handleRoot() {
	// Начальная страница

	LED ? neopixelWrite(RGB_BUILTIN, 16, 16, 16) : digitalWrite(RGB_BUILTIN, LOW);
	LED = !LED;

  	server.send(200, "text/plain", "OK");
}

void setup() {
	// Setup configuration
	Serial.begin(115200);
	digitalWrite(RGB_BUILTIN, LOW);
  
	// Запускаем локальный сервер 
	// WiFi.softAP("Frontend", "Backend;");
	IPAddress localIP(192,168,222,11);
	IPAddress gw(192,168,222,11);
	IPAddress mask(255,255,255,255);
	WiFi.softAPConfig(localIP, gw, mask);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	
	server.begin();
	Serial.println("HTTP server started");
    
    // Serial.println("WiFi connected");
    // Serial.println("IP address: ");
    // Serial.println(WiFi.localIP());

	// Установка статического IP адреса
	IPAddress local_IP(192, 168, 1, 100);
	IPAddress gateway(192, 168, 1, 1);
	IPAddress subnet(255, 255, 255, 0);

	WiFi.config(local_IP, gateway, subnet);

	// Подключение к Wi-Fi
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Connecting to WiFi...");
	}
	Serial.println("Connected to WiFi");
    
	// Инициализация обработчиков запросов сервера
    server.on("/switch", handleRoot);
	neopixelWrite(RGB_BUILTIN, 0, 16, 0);
  
	server.begin();
}

void loop() {
	server.handleClient();
}