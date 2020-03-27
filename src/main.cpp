#include "main.hpp"

const char* ssid = SSID;
const char* password = PSK;

ESP8266WebServer server(80);
DHT dht(SENSOR_PIN, DHT22);

IPAddress staticIP(192, 168, 0, 220); //ESP static ip
IPAddress gateway(192, 168, 0, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS

WiFiClient client;

void handleNotFound();
float getTargetTemperature();

void setup(void) {
	Serial.begin(115200);
	WiFi.mode(WIFI_STA);
	WiFi.config(staticIP, gateway, subnet, dns);
	WiFi.begin(ssid, password);

	dht.begin();

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	server.onNotFound(handleNotFound);

	server.on("/tempF", []() {
		server.send(200, "text/plain", String(dht.readTemperature(true)));
	});

	server.on("/tempC", []() {
		server.send(200, "text/plain", String(dht.readTemperature()));
	});

	server.on("/humidity", []() {
		server.send(200, "text/plain", String(dht.readHumidity()));
	});

	server.onNotFound(handleNotFound);
	server.begin();

	Serial.println("HTTP server started");
}

void loop(void) {
	server.handleClient();
	delay(1);
}

float getTargetTemperature() {
	HTTPClient http;
	http.begin(client, OPENWEATHERMAP_API_URL);

	int httpCode = http.GET();
	if (httpCode > 0) {
		if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
			String payload = http.getString();
//			Serial.println(payload);
			StaticJsonDocument<500> data;
			deserializeJson(data, payload);
			String temp_max_F = data["main"]["temp_max"].as<String>();
			return temp_max_F.toFloat();
		}
	}
	Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
	return 0.0;
}

void handleNotFound() {
	server.send(404, "text/plain",
			"Hello! \n"
			"Use the following paths: \n"
			"/tempF \n"
			"/tempC \n"
			"/humidity  \n");
}