
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

	// Возвращаем ID дрона (количество подключенных +1)
	droneCount++;
	server.send(200, "text/plain", "Data: id="+String(droneCount));
}


String adminHtml() {
	// Я не знаю как еще было записать HTML код.
	// Файлы я не смог прочитать (около суток пытался это сделать)

	String sh = "<!DOCTYPE HTML>";
	sh += "<html>";
	sh += "<head>";
	sh += "<title>Admin panel</title>";
	sh += "<meta charset=\"UTF-8\">"; // Очень важно. иначе Русские буквы превратятся в монстров
	sh += "</head>";
	sh += "<body>";
	sh += "<h1>Admin panel</h1>";
	sh += "<h3>Drone count: " + String(droneCount) + "</h3><hr>";

	// Добавляем управление каждым дроном
	for (int i = 0; i < droneCount; i++) {
		sh += "<div><h4>Отправить команду дрону №"+String(i+1)+"</h4><input id='d"+String(i+1)+"' type='color'>";
		sh += "<button onclick=\"window.location.href = location.protocol + '//' + location.host + '/cmnd?drone="+String(i+1);
		sh += "&' + getColor('d"+String(i+1)+"')\">Submit</button></div><hr>";
	}
	
	// Управление всеми дронами
	sh += "<div><h4>Отправить команду всем дронам</h4><input id='d0' type='color'>";
	sh += "<button onclick=\"window.location.href = location.protocol + '//' + location.host + '/cmnd?drone=0";
	sh += "&' + getColor('d0')\">Submit</button></div><hr>";

	sh += "</body>";

	// Функция, которая конвертирует HEX значение выбора цвета в RGB
	sh += "<script type=\"text/javascript\">";
	sh += "function getColor(idd) {";
	sh += "var el = document.getElementById(idd).value;";
	sh += "return `r=${parseInt(el.substr(1,2), 16)}&g=${parseInt(el.substr(3,2), 16)}&b=${parseInt(el.substr(5,2), 16)}`";
	sh += "}</script>";

	sh += "</html>";

	return sh;
}

void admin() {
	// Отдача страницы админки
    server.send(200, "text/html", adminHtml());
}

void cmnd() {
	// Отдача команды одному или всему дрону

    int droneId = server.arg("drone").toInt(); // ID дрона, которому передать команду. 0 - означает всем
	String request = server.uri()+"?";

	for (int i = 0; i < server.args(); i++){
		// Вывод аргументов по отдельности
		// Serial.println(server.argName(i) + " = " + server.arg(i));

		request += server.argName(i) + "=" + server.arg(i) + "&";
	}
	
	// Сборка запроса, чтобы отправить его дальше
	request = request.substring(0, request.length() - 1);
	Serial.println("Request: " + request);
    
	// Если команда отправляется всем дронам (id = 0) или корректный ID дрона, выполняем её
	if (droneId == 0 || droneId == ID) {
		Serial.println("Выполнение команды.");
		neopixelWrite(RGB_BUILTIN, server.arg("r").toInt(), server.arg("g").toInt(), server.arg("b").toInt());

	    // Останавливаем цепь, если команда была конкретно этому дрону
		if (droneId != 0){
			admin();
			return;
		}
	}

    // Отправка команды последующим дронам
	// Поиск следующего дрона (если есть дрон с ID+1)
	char* ssid = strdup(("Drone-" + String(ID+1)).c_str()); 
	if (serchWiFi(ssid)) {

		// Подключение к WiFi
		Serial.println("Connecting");
		WiFi.begin(ssid);
		while (WiFi.status() != WL_CONNECTED){
			Serial.print(".");
			delay(500);
		}
		Serial.println("\nConacted!");


		// Дублируем запрос
		WiFiClient wfc;
		wfc.connect(strdup(("192.168.222." + String(11 + ID)).c_str()), 80);
		// wfc.connect("192.168.222.11", 80);
		wfc.println("GET "+request+" HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");
		delay(500);
		wfc.stop();
		Serial.println("Request sent.");
	}

    admin();
}

void serverUp () {
	// Создание точки доступа

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
		server.on("/cmnd", cmnd);
		neopixelWrite(RGB_BUILTIN, 8, 0, 8);
	} else {
		serverUp();
		// Инициализация обработчиков GET запросов
		server.on("/register", registerDrone);
		server.on("/", admin);
		server.on("/cmnd", cmnd);
		neopixelWrite(RGB_BUILTIN, 8, 8, 8);
	}

}

void loop() {
  	server.handleClient();
}
