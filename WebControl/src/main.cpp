#include <Arduino.h>
#include <painlessMesh.h>
#include <WiFi.h>
#include <WebServer.h>

#define ssid "WiFi Web Control"

WebServer server(80);

const int ledPin = 2;

void handleRoot() {
	// Начальная страница
  	server.send(200, "text/html", "<h1>Hello from ESP32!</h1><p><a href='/on'>Turn On LED</a></p>");
}

void handleOn() {
	// Включение светодиода
	neopixelWrite(RGB_BUILTIN, 0, 0, 32);
  	server.send(200, "text/html", "<h1>LED is On</h1><p><a href='/off'>Turn Off LED</a></p>");
}

void handleOff() {
	// Отключение светодиода
	digitalWrite(RGB_BUILTIN, LOW);
	server.send(200, "text/html", "<h1>LED is Off</h1><p><a href='/on'>Turn On LED</a></p>");
}

void setup() {
	// Setup configuration
	Serial.begin(115200);
	pinMode(ledPin, OUTPUT);
  
	// Запускаем локальный сервер 
	WiFi.softAP(ssid);
	IPAddress localIP(192,168,222,11);
	IPAddress gw(192,168,222,11);
	IPAddress mask(255,255,255,0);
	WiFi.softAPConfig(localIP, gw, mask);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/", handleRoot);
	server.begin();
	Serial.println("HTTP server started");
    
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
	// Инициализация обработчиков запросов сервера
    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
  
	server.begin();
}

void loop() {
	server.handleClient();
}