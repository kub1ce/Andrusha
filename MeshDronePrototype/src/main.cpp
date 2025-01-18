
#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>


#define isDebugMode false // Если true, то включается режим отладки. Чтобы плата начала работать - нужно отправить любое сообщение в Serial порт


int ID = 2; // Номер дрона
int droneCount = 2; // Количество дронов. Необходимо только для главного дрона.

WebServer server(80);

WiFiClient wfc;

// =====

String adminHtml();

bool serchWiFi(char* ssid) {
	// Поиск точки с именем
	Serial.print(ssid);

	// Сканирование доступных сетей. Занимает время..
	int numNetworks = WiFi.scanNetworks();

	// Поиск нужной сети с подходящем названием (ssid)
	for (int i = 0; i < numNetworks; i++) {
		if (WiFi.SSID(i) == ssid) {
            Serial.println(" found");
			return true;
		}
	}
	Serial.println(" not found");

	return false;
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
	sh += "<body style='font-family: monospace;'>"; // шрифт по вкусу
	sh += "<h1><a href='/'>Admin panel</a></h1>";
	sh += "<h3>Drone count: <b id='droneCount'>" + String(droneCount) + "</b></h3><hr>";

	// Добавляем управление каждым дроном
	for (int i = 0; i < droneCount; i++) {
		sh += "<div><h4>Отправить команду дрону №"+String(i+1)+"</h4><input id='d"+String(i+1)+"' type='color'>";
		sh += "<button onclick=\"window.location.href = location.protocol + '//' + location.host + '/cmnd?drone="+String(i+1);
		sh += "&' + getColor('d"+String(i+1)+"')\">Submit</button></div>";

		if (i+1 == droneCount)
			sh += "<hr style='border-color: orangered'>";
		else
			sh += "<hr>";
	}
	
	// Управление всеми дронами
	sh += "<div><h4>Отправить команду всем дронам</h4><input id='d0' type='color'>";
	sh += "<button onclick=\"window.location.href = location.protocol + '//' + location.host + '/cmnd?drone=0";
	sh += "&' + getColor('d0')\">Submit</button></div>";

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
	// Отдача команды одному дрону или всем сразу

    int droneId = server.arg("drone").toInt(); // ID дрона, которому передать команду. 0 - означает всем

	// Сборка запроса, чтобы отправить его дальше
	String request = server.uri()+"?";
	for (int i = 0; i < server.args(); i++){
		// Вывод аргументов по отдельности
		if (isDebugMode) Serial.println(server.argName(i) + " = " + server.arg(i));

		request += server.argName(i) + "=" + server.arg(i) + "&";
	}
	request = request.substring(0, request.length() - 1);

	if (isDebugMode) Serial.println("Request: " + request);
    
	// Если команда отправляется всем дронам (id = 0) или корректный ID дрона, выполняем её
	if (droneId == 0 || droneId == ID) {
		Serial.println("Выполнение команды.");
		if (server.arg("r").toInt() == 0 && server.arg("g").toInt() == 0 && server.arg("b").toInt() == 0)
			digitalWrite(RGB_BUILTIN, LOW);
		else	
			neopixelWrite(RGB_BUILTIN, server.arg("r").toInt(), server.arg("g").toInt(), server.arg("b").toInt());

	    // Останавливаем цепь, если команда была конкретно этому дрону
		if (droneId != 0 || ID == droneCount){
			ID == 1 ? admin() : server.send(200);
			return;
		}
	}

	// Проверка подключения к следующему дрону
	if (WiFi.status() != WL_CONNECTED && ID < droneCount){
		// Реконнект, если WiFi не подключен
		Serial.println("Reconnecting.");
		char* ssid = strdup(("Drone-" + String(ID+1)).c_str()); 
		if (serchWiFi(ssid)) {
			Serial.println("Connecting");
			WiFi.begin(ssid);
			while (WiFi.status() != WL_CONNECTED){
				Serial.print(".");
				delay(500);
			}
			Serial.println("\nConected!");
		} else {
			Serial.println("Drone not found!");
            return;
		}
	} else {
		if (isDebugMode) Serial.println("Already connected to " + WiFi.SSID());
	}


    // Отправка команды последующим дронам
	// Дублируем запрос
	wfc.connect(strdup(("192.168.222." + String(11 + ID)).c_str()), 80);
	// wfc.connect("192.168.222.11", 80);
	wfc.println("GET "+request+" HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");
	Serial.println("Request sent.");

    // Возврат ответа
	ID==1 ? admin() : server.send(200);
	return;

}

void serverUp () {
	// Создание точки доступа

	WiFi.softAP("Drone-"+String(ID));

	IPAddress localIP(192, 168, 222, 11+ID-1);
	IPAddress gw(192, 168, 222, 11+ID-1);
	IPAddress mask(255, 255, 255, 0);
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
	digitalWrite(RGB_BUILTIN, LOW);

	if (isDebugMode){
		// ! Отложенный запуск. см комментарий к isDebugMode
		while (!Serial.available()) { }
		Serial.println(">>> " + Serial.readString());
	}

    // * 2.
	if (ID != 1) {
		// Запуск WiFi
		Serial.println("Запуск WiFi с именем Drone-"+String(ID));
		serverUp();
		neopixelWrite(RGB_BUILTIN, 8, 0, 8);
	} else {
		serverUp();
		// Инициализация обработчиков GET запросов
		server.on("/", admin);
		neopixelWrite(RGB_BUILTIN, 8, 8, 8);
	}

	server.on("/cmnd", cmnd);

}

void loop() {
  	server.handleClient();
}