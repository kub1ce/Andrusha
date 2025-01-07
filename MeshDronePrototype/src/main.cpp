
#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>

/*
	"+" - реализовано
	"-" - планируется 


	1. Поиск точки с именем Drone-1
	+ Подключение к найденной точке
	+ Запрос /register => получение ID
	+ Запуск WiFi с именем Drone-ID ↑
	
	Если точки нет:
		+ Запуск Wifi с именем Drone-1
		+ реализация метода /register
		- "админ панель"
			- Отображение подключенных дронов
			- Отдача команды одному
			- Отдача всем сразу
		- Отдача команд одному дрону или всем сразу
*/


int ID = 1; // Номер дрона
int droneCount = 1; // Количество дронов. Необходимо только для главного дрона.

WebServer server(80);

// =====

bool serchWiFi(char* ssid) {
	// Поиск точки с именем
	Serial.print(ssid);

	int numNetworks = WiFi.scanNetworks();

	for (int i = 0; i < numNetworks; i++) {
		if (WiFi.SSID(i) == ssid) {
            Serial.println(" found");
			return true;
		}
	}
	Serial.println(" not found");

	return false;
}

void parseArgumentsFromRespond(const String& header) {
	// Парсинг ответа от сервера, получение и обновление ID

    size_t dataIndex = header.indexOf("Data: id=");
	size_t idIndex = dataIndex + 9; // Длина "Data: id=" равна 8
	String idStr = header.substring(idIndex);
	int idEnd = idStr.toInt();
	String idValue = idStr.substring(0, idEnd);

	ID = idValue.toInt();
}

void getID() {
    // Запрос /register => получение ID

	Serial.println("Drone-"+String(ID));
	Serial.print("Connecting");
	WiFi.begin("Drone-"+String(ID));
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		neopixelWrite(RGB_BUILTIN, 32, 0, 0);
		delay(1000);
		digitalWrite(RGB_BUILTIN, LOW);
	}

	Serial.println("\nConnected!");
	neopixelWrite(RGB_BUILTIN, 0, 32, 0);
	delay(500);
	digitalWrite(RGB_BUILTIN, LOW);


	// Отправка GET-запроса на сервер
	WiFiClient wfc;
	wfc.connect("192.168.222.11", 80);
	wfc.println("GET /register HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");
	
	delay(500);
	
	// Чтение ответа
	String response = "";
    while (wfc.connected()) {
        while (wfc.available()) {
            char c = wfc.read();
			response += c;
        }
    }

	wfc.stop();
	parseArgumentsFromRespond(response);
}

void registerDrone() {
	// Реализация метода /register
	server.send(200, "text/plain", "Data: id="+String(droneCount+1));
}


void serverUp () {
	// Запуск локального сервера на статичном IP
	WiFi.softAP("Drone-"+String(ID));
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


  	server.begin();
}

void setup() {
	// Setup configuration
	Serial.begin(115200);

	// * 1. Поиск точки с именем Drone-1

	char* ssid = (char*)"Drone-1";
	bool isDrone1Exists = serchWiFi(ssid);

	// Индикация существования точки
	if (isDrone1Exists)
		neopixelWrite(RGB_BUILTIN, 0, 32, 0);
	else
		neopixelWrite(RGB_BUILTIN, 32, 0, 0);
	
	delay(240);
	digitalWrite(RGB_BUILTIN, LOW);

    // * 2.
	if (isDrone1Exists) {
		// Получение нового Id устройства
		getID();
		Serial.println("Новый Id устройства: "+String(ID));
		Serial.println("Запуск WiFi с именем Drone-"+String(ID));
		// Запуск WiFi
		serverUp();		
	} else {
		serverUp();
		// Инициализация обработчиков GET запросов
		server.on("/register", registerDrone);
	}

}

void loop() {
  	server.handleClient();

	if (ID == 1) 
		neopixelWrite(RGB_BUILTIN, 16, 16, 16);
	else
		neopixelWrite(RGB_BUILTIN, 4, 0, 4);

	delay(240);
	digitalWrite(RGB_BUILTIN, LOW);
	delay(240);
}