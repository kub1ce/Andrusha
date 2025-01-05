
#include <Arduino.h>
#include <painlessMesh.h>

#include <WiFi.h>
#include <WebServer.h>

#define ssid "R2-D2"
#define wifiToConnect "WiFi Web Control"

WebServer server(80);

const int ledPin = 2;

void handleRoot() {
  server.send(200, "text/html", "<h1>Hello from ESP32!</h1><p><a href='/on'>Turn On LED</a></p>");
}

void handleOn() {
	// Включение светодиода

	// Подключение к WiFi
	neopixelWrite(RGB_BUILTIN, 0, 32, 32);
	delay(240);
	digitalWrite(RGB_BUILTIN, LOW);

	WiFi.begin(wifiToConnect);
	while (WiFi.status() != WL_CONNECTED) {
		neopixelWrite(RGB_BUILTIN, 32, 0, 0);
		delay(500);
		digitalWrite(RGB_BUILTIN, LOW);
	}
	neopixelWrite(RGB_BUILTIN, 0, 32, 0);
	delay(1000);
	digitalWrite(RGB_BUILTIN, LOW);
	WiFiClient wfc;

	// Отправка GET-запроса на сервер
	wfc.connect("192.168.222.11", 80);
	wfc.println("GET /on HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");

	// Реагирование на получение ответа от сервера
	neopixelWrite(RGB_BUILTIN, 32, 32, 0);
	delay(1000);
	digitalWrite(RGB_BUILTIN, LOW);

	// Возвращаем ответ пользователю с кодом 200
  	server.send(200, "text/html", "<h1>LED is On</h1><p><a href='/off'>Turn Off LED</a></p>");
}

void handleOff() {
	// Выключение светодиода

	// Подключение к WiFi
	neopixelWrite(RGB_BUILTIN, 0, 32, 32);
	delay(240);
	digitalWrite(RGB_BUILTIN, LOW);

	WiFi.begin(wifiToConnect);
	while (WiFi.status() != WL_CONNECTED) {
		neopixelWrite(RGB_BUILTIN, 0, 32, 32);
		delay(500);
		digitalWrite(RGB_BUILTIN, LOW);
	}
	neopixelWrite(RGB_BUILTIN, 0, 32, 0);
	delay(1000);
	digitalWrite(RGB_BUILTIN, LOW);
	WiFiClient wfc;

	// Отправка GET-запроса на сервер
	wfc.connect("192.168.222.11", 80);
	wfc.println("GET /off HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");

	// Реагирование на получение ответа от сервера
	neopixelWrite(RGB_BUILTIN, 32, 32, 0);
	delay(1000);
	digitalWrite(RGB_BUILTIN, LOW);

	// Возвращаем ответ пользователю с кодом 200
  	server.send(200, "text/html", "<h1>LED is Off</h1><p><a href='/on'>Turn ON LED</a></p>");
}

void setup() {
	// Setup configuration
	Serial.begin(115200);
	pinMode(ledPin, OUTPUT);

	// Запуск локального сервера на статичном IP
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
    
	// Инициализация обработчиков GET запросов
    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
  
  server.begin();
}

void loop() {
  server.handleClient();
}
