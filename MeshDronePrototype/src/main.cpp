
#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>


#define isDebugMode false // Если true, то включается режим отладки. Чтобы плата начала работать - нужно отправить любое сообщение в Serial порт


int ID = 1; // Номер дрона
int droneCount = 1; // Количество дронов. Необходимо только для главного дрона.

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

void parseArgumentsFromRespond(const String& header) {
	// Парсинг ответа от сервера, получение и обновление ID

    size_t dataIndex = header.indexOf("Data: id="); // Ищем индекс необходимого заголовка в ответе
	size_t idIndex = dataIndex + 9; // Длина "Data: id=" равна 8
	String idStr = header.substring(idIndex); // Обрезаем строку так, чтобы осталось только число
	int idEnd = idStr.toInt();
	String idValue = idStr.substring(0, idEnd);

	ID = idValue.toInt(); // Записываем новый ID
}

void getID() {
    // Запрос /register => получение ID

	// Подключение к WiFi Drone-1
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
	// WiFiClient wfc;
	wfc.connect("192.168.222.11", 80);
	wfc.println("GET /register HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");

	// Немного ждём, чтобы все пакеты успешно дошли
	delay(500);
	
	// Чтение ответа
	String response = "";
    while (wfc.connected()) {
        while (wfc.available()) {
            char c = wfc.read();
			response += c;
        }
    }

	// Отключаемся от 1го дрона
	wfc.stop();

	// Парсим ответ и записываем новый ID
	parseArgumentsFromRespond(response);
}

void registerDrone() {
	// Реализация метода /register

	// Возвращаем ID дрона (количество подключенных +1)
	Serial.println("Регистрация нового дрона");
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
	sh += "<body style='font-family: monospace;'>"; // шрифт по вкусу
	sh += "<h1><a href='/'>Admin panel</a></h1>";
	// sh += "<h4><a href='/updateConnection'>Update Connection</a></h4>";
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

	// Функция, которая обновляет значение DroneCount
	sh += "<script type=\"text/javascript\">";
	sh += "function updateDroneCount() {";
	sh += "var el = document.getElementById('droneCount')";
	sh += "fetch('/droneCount')";
	sh += ".then(responce => responce.text())";
	sh += ".then(data => {el.innerText == data ? {} : el.innerText = data, el.style.color = 'red'})";
	sh += "}";
	sh += "setInterval(updateDroneCount, 1000);";
    sh += "</script>";

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
		if (droneId != 0){
			ID == 1 ? admin() : server.send(200);
			return;
		}
	}

	// Проверка подключения к следующему дрону
	if (WiFi.status() != WL_CONNECTED){
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
	wfc.println("GET "+request+" HTTP/1.1\r\nHost: 192.168.222.11\r\nUser-Agent: ESP32\r\nConnection: close\r\n\r\n");
	Serial.println("Request sent.");

    // Возврат ответа
	ID==1 ? admin() : server.send(200);

}

// ? New

void handleDroneCount() {
	server.send(200, "text/plain", String(droneCount));
}

// ? New

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

	// Иногда IP может быть неверным => другая плата не сможет подключиться, чтобы передать команду
	// Если светодиод будет мигать красным цветом, то значит - это произошло( 
	// В таком случае следует повторно перезапустить все платы выжидая более длительное время между подключениями

	// if (String(WiFi.localIP()) != ("192.168.222." + String(11 + ID - 1))) {
	// 	while (true)
	// 	{
	// 		neopixelWrite(RGB_BUILTIN, 8, 0, 0);
	// 		delay(240);
	// 		digitalWrite(RGB_BUILTIN, LOW);
	// 		delay(240);
	// 	}
		
	// }

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
		neopixelWrite(RGB_BUILTIN, 8, 0, 8);
	} else {
		serverUp();
		// Инициализация обработчиков GET запросов
		server.on("/register", registerDrone);
		server.on("/", admin);
		server.on("/droneCount", HTTP_GET, handleDroneCount);
		neopixelWrite(RGB_BUILTIN, 8, 8, 8);
	}

	server.on("/cmnd", cmnd);

}

void loop() {
  	server.handleClient();
}