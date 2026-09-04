#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <DHT.h>

// Настройки WiFi
const char* ssid = "ENA-WiFi";
const char* password = "94556870";

// Пины для ESP32
#define ONE_WIRE_BUS 4   // DS18B20
#define DHTPIN 5         // DHT11
#define DHTTYPE DHT11

// Объекты датчиков
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
DHT dht(DHTPIN, DHTTYPE);

// Веб-сервер
WebServer server(80);

// Глобальные переменные
float tempDS = -127.0;
float tempDHT = -127.0;
float humidity = -1.0;
bool dsFound = false;

// HTML-страница
String getHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Метеостанция</title>";
    html += "<style>";
    html += "*{margin:0;padding:0;box-sizing:border-box}";
    html += "body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}";
    html += ".container{background:rgba(255,255,255,0.95);border-radius:20px;padding:30px;max-width:500px;width:100%;box-shadow:0 20px 60px rgba(0,0,0,0.3)}";
    html += "h1{text-align:center;color:#333;margin-bottom:30px;font-size:28px}";
    html += ".sensor-card{background:#f8f9fa;border-radius:15px;padding:20px;margin-bottom:20px;transition:transform 0.3s}";
    html += ".sensor-card:hover{transform:translateY(-2px)}";
    html += ".sensor-title{color:#666;font-size:14px;text-transform:uppercase;letter-spacing:1px;margin-bottom:10px}";
    html += ".sensor-value{font-size:52px;font-weight:bold;color:#333}";
    html += ".sensor-unit{font-size:24px;color:#666;margin-left:5px}";
    html += ".temp-ds{color:#4a90e2}";
    html += ".temp-dht{color:#e67e22}";
    html += ".humidity{color:#27ae60}";
    html += ".status{text-align:center;color:#999;font-size:12px;margin-top:20px}";
    html += ".error{color:#e74c3c;font-size:18px}";
    html += "</style>";
    html += "</head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🌡️ Метеостанция</h1>";
    
    // DS18B20
    html += "<div class='sensor-card'>";
    html += "<div class='sensor-title'>За • бортом</div>";
    if(dsFound) {
        html += "<div class='sensor-value temp-ds'>" + String(tempDS, 1) + "<span class='sensor-unit'>°C</span></div>";
    } else {
        html += "<div class='sensor-value error'>⚠️ Нет датчика</div>";
    }
    html += "</div>";
    
    // DHT11
    html += "<div class='sensor-card'>";
    html += "<div class='sensor-title'>В • доме</div>";
    if(tempDHT > -50) {
        html += "<div class='sensor-value temp-dht'>" + String(tempDHT, 1) + "<span class='sensor-unit'>°C</span></div>";
        html += "<div class='sensor-value humidity' style='margin-top:10px'>💧 " + String(humidity, 1) + "<span class='sensor-unit'>%</span></div>";
    } else {
        html += "<div class='sensor-value error'>⚠️ Ошибка чтения</div>";
    }
    html += "</div>";
    
    html += "<div class='status'>Обновляется каждые 3 секунды</div>";
    html += "</div>";
    
    // JavaScript для автобновления
    html += "<script>";
    html += "setInterval(() => {";
    html += "  fetch('/data')";
    html += "    .then(res => res.json())";
    html += "    .then(data => {";
    html += "      const ds = document.querySelector('.temp-ds');";
    html += "      if(ds) ds.innerHTML = data.ds + '<span class=\\'sensor-unit\\'>°C</span>';";
    html += "      const dht = document.querySelector('.temp-dht');";
    html += "      if(dht) dht.innerHTML = data.dht + '<span class=\\'sensor-unit\\'>°C</span>';";
    html += "      const hum = document.querySelector('.humidity');";
    html += "      if(hum) hum.innerHTML = '💧 ' + data.hum + '<span class=\\'sensor-unit\\'>%</span>';";
    html += "    })";
    html += "    .catch(() => {});";
    html += "}, 3000);";
    html += "</script>";
    
    html += "</body></html>";
    return html;
}

// Обработчик данных
void handleData() {
    String json = "{";
    json += "\"ds\":" + String(tempDS, 1) + ",";
    json += "\"dht\":" + String(tempDHT, 1) + ",";
    json += "\"hum\":" + String(humidity, 1);
    json += "}";
    server.send(200, "application/json", json);
}

// Обработчик главной страницы
void handleRoot() {
    server.send(200, "text/html", getHTML());
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n==================================");
    Serial.println("  Метеостанция ESP32");
    Serial.println("==================================\n");
    
    // Инициализация датчиков
    Serial.println("Инициализация датчиков...");
    DS18B20.begin();
    dht.begin();
    
    // Подключение к WiFi
    Serial.print("Подключение к WiFi");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Подключено к WiFi!");
        Serial.print("📡 IP-адрес: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n❌ Ошибка подключения к WiFi!");
        return;
    }
    
    // Настройка веб-сервера
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
    
    Serial.println("🌐 Веб-сервер запущен");
    Serial.println("Откройте в браузере: http://" + WiFi.localIP().toString());
    Serial.println("==================================\n");
}

void loop() {
    server.handleClient();
    
    static unsigned long lastRead = 0;
    if (millis() - lastRead > 3000) {
        lastRead = millis();
        
        // Чтение DS18B20
        DS18B20.requestTemperatures();
        tempDS = DS18B20.getTempCByIndex(0);
        dsFound = (tempDS != DEVICE_DISCONNECTED_C && tempDS > -50);
        
        // Чтение DHT11
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        if (!isnan(h) && !isnan(t)) {
            humidity = h;
            tempDHT = t;
        }
        
        // Вывод в Serial
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 5000) {
            lastPrint = millis();
            Serial.print("🌡️ DS18B20: ");
            if (dsFound) {
                Serial.print(tempDS, 1);
                Serial.print("°C, ");
            } else {
                Serial.print("Нет датчика, ");
            }
            Serial.print("DHT11: ");
            if (tempDHT > -50) {
                Serial.print(tempDHT, 1);
                Serial.print("°C, ");
                Serial.print(humidity, 1);
                Serial.println("%");
            } else {
                Serial.println("Ошибка чтения");
            }
        }
    }
}