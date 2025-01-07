#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>


#define ssid "ParamPassing"

WebServer server(80);

const int ledPin = 2;
const char* text = "<form id='colorForm' action='/submit-color' method='get'><input type='color' id='colorPicker' name='color'><button type='submit'>Применить цвет</button></form><script>document.getElementById('colorForm').addEventListener('submit', function(event) {event.preventDefault();var colorValue = document.getElementById('colorPicker').value;var r = parseInt(colorValue.substr(1, 2), 16);var g = parseInt(colorValue.substr(3, 2), 16);var b = parseInt(colorValue.substr(5, 2), 16);var url = '/submit-color?r=' + r + '&g=' + g + '&b=' + b;window.location.href = url;});</script>";

void handleRoot() {
	server.send(200, "text/html", text);
}

void handleSubmitColor() {
	if (server.args() > 0) {
		int r = server.arg("r").toInt();
		int g = server.arg("g").toInt();
		int b = server.arg("b").toInt();

		neopixelWrite(RGB_BUILTIN, r, g, b);
	}

	handleRoot();
}

void setup() {
	// Setup configuration
	Serial.begin(115200);
	neopixelWrite(RGB_BUILTIN, 32, 0, 32);
	delay(100);
	digitalWrite(RGB_BUILTIN, LOW);
  
	// Запускаем локальный сервер 
	WiFi.softAP(ssid);
	IPAddress localIP(192,168,222,11);
	IPAddress gw(192,168,222,11);
	IPAddress mask(255,255,255,0);
	WiFi.softAPConfig(localIP, gw, mask);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.begin();
	Serial.println("HTTP server started");
    
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
	// Инициализация обработчиков запросов сервера
    server.on("/", handleRoot);
	server.on("/submit-color", HTTP_GET, handleSubmitColor);
  
	server.begin();
}

void loop() {
	server.handleClient();
}