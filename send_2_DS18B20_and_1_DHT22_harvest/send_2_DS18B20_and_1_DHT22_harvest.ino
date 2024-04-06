#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define CONSOLE Serial
#define INTERVAL_MS 10000 // Send data every 10 seconds
#define ENDPOINT "uni.soracom.io"
#define SKETCH_NAME "send_multi_sensor_data_to_lagoon"
#define VERSION "1.3"

/* for LTE-M Shield for Arduino */
#define RX 10
#define TX 11
#define BAUDRATE 9600
#define BG96_RESET 15

SoftwareSerial LTE_M_shieldUART(RX, TX);
TinyGsm modem(LTE_M_shieldUART);
TinyGsmClient ctx(modem);

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Settings for DallasTemperature sensors
#define TRAY_ONE_WIRE_BUS 2 // Pin for the tray temperature sensor
#define TUNNEL_ONE_WIRE_BUS 3 // Pin for the tunnel temperature sensor
OneWire oneWireTray(TRAY_ONE_WIRE_BUS);
OneWire oneWireTunnel(TUNNEL_ONE_WIRE_BUS);
DallasTemperature traySensor(&oneWireTray);
DallasTemperature tunnelSensor(&oneWireTunnel);

void setup() {
  CONSOLE.begin(115200);
  LTE_M_shieldUART.begin(BAUDRATE);

  CONSOLE.print(F("Welcome to "));
  CONSOLE.print(SKETCH_NAME);
  CONSOLE.print(F(" "));
  CONSOLE.println(VERSION);
  delay(3000);

  pinMode(BG96_RESET, OUTPUT);
  digitalWrite(BG96_RESET, LOW);
  delay(300);
  digitalWrite(BG96_RESET, HIGH);
  delay(300);
  digitalWrite(BG96_RESET, LOW);
  CONSOLE.println(F("Modem reset done."));

  modem.restart();
  delay(500);

  modem.gprsConnect("soracom.io", "sora", "sora");

  while (!modem.isNetworkConnected()) {
    delay(500);
  }

  CONSOLE.println(F("Network connected."));

  dht.begin();
  traySensor.begin();
  tunnelSensor.begin();
}

void loop() {
  traySensor.requestTemperatures();
  float trayTemp = traySensor.getTempCByIndex(0);
  tunnelSensor.requestTemperatures();
  float tunnelTemp = tunnelSensor.getTempCByIndex(0);
  float ambientTemp = dht.readTemperature();
  float humidity = dht.readHumidity();

  String payload = "{\"tray_temperature\":" + String(trayTemp, 2) +
                   ",\"tunnel_temperature\":" + String(tunnelTemp, 2) +
                   ",\"ambient_temperature\":" + String(ambientTemp, 2) +
                   ",\"humidity\":" + String(humidity, 2) + "}";

  CONSOLE.println(payload);

  if (!ctx.connect(ENDPOINT, 80)) {
    CONSOLE.println(F("Connection failed."));
    delay(3000);
    return;
  }

  ctx.print(F("POST / HTTP/1.1\r\n"));
  ctx.print(F("Host: ")); ctx.print(ENDPOINT); ctx.println();
  ctx.println(F("Content-Type: application/json"));
  ctx.print(F("Content-Length: ")); ctx.println(payload.length());
  ctx.println();
  ctx.println(payload);

  while (ctx.connected()) {
    if (ctx.available()) {
      String line = ctx.readStringUntil('\n');
      if (line == "\r") {
        CONSOLE.println(F("Headers received."));
        break;
      }
    }
  }
  ctx.stop();
  CONSOLE.println(F("Data sent."));

  delay(INTERVAL_MS);
}
