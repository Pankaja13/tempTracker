#include "main.hpp"

const char* ssid = SSID;
const char* password = PSK;
const long samplePeriod = 10000L;

ESP8266WebServer server(80);
DHT dht(SENSOR_PIN, DHT22);
SoftwareSerial sensor(CO2_RX, CO2_TX); // RX, TX

long lastSampleTime = 0;

IPAddress staticIP(192, 168, 0, 200); //ESP static ip
IPAddress gateway(192, 168, 0, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS

WiFiClient client;

void handleNotFound();
float getTargetTemperature();
void sendNotification();
int readPPMSerial();


void setup(void) {
	Serial.begin(115200);

	sensor.begin(9600);

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

	server.on("/co2_ppm", []() {
		server.send(200, "text/plain", String(readPPMSerial()));
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

void sendNotification() {
	HTTPClient http;
	http.begin(client, NOTIFICATION_URL);

	int httpCode = http.GET();
	if (httpCode > 0) {
		if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
			String payload = http.getString();
			return;
		}
	}
	Serial.println("Notification request failed");
}

void handleNotFound() {
	server.send(404, "text/plain",
			"Hello! \n"
			"Use the following paths: \n"
			"/tempF \n"
			"/tempC \n"
			"/humidity  \n"
			"/co2_ppm  \n");
}

int readPPMSerial() {
	if (millis() < 1000*60*3){ // 3 minute heat-up time
		return -1;
	}

	const byte requestReading[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
	byte result[9];

	for (unsigned char i : requestReading) {
		sensor.write(i);
	}
	while (sensor.available() < 9) {}; // wait for response
	for (unsigned char & i : result) {
		i = sensor.read();
	}
	int high = result[2];
	int low = result[3];
	return high * 256 + low;
}