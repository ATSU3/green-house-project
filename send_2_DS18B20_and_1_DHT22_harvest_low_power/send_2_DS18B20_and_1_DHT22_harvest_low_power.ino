#define TINY_GSM_MODEM_BG96
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <LowPower.h>

#define CONSOLE Serial
#define INTERVAL_MS 300000
#define SERVER "harvest.soracom.io"
#define PORT 8514

#define RX 10
#define TX 11
#define BAUDRATE 9600
#define BG96_RESET 15

SoftwareSerial LTE_M_shieldUART(RX, TX);
TinyGsm modem(LTE_M_shieldUART);
TinyGsmClient client(modem);

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define TRAY_ONE_WIRE_BUS 2
#define TUNNEL_ONE_WIRE_BUS 3
OneWire oneWireTray(TRAY_ONE_WIRE_BUS);
OneWire oneWireTunnel(TUNNEL_ONE_WIRE_BUS);
DallasTemperature traySensor(&oneWireTray);
DallasTemperature tunnelSensor(&oneWireTunnel);

void setup() {
  CONSOLE.begin(115200);
  LTE_M_shieldUART.begin(BAUDRATE);

  CONSOLE.println(F("Starting..."));

  pinMode(BG96_RESET, OUTPUT);
  digitalWrite(BG96_RESET, HIGH);
  delay(300);
  digitalWrite(BG96_RESET, LOW);
  delay(10000);

  if (!modem.restart()) {
    CONSOLE.println(F("Failed to restart modem"));
    return;
  }

  String modemInfo = modem.getModemInfo();
  CONSOLE.print(F("Modem: "));
  CONSOLE.println(modemInfo);

  if (!modem.gprsConnect("soracom.io", "sora", "sora")) {
    CONSOLE.println(F("Failed to connect"));
    return;
  }

  CONSOLE.println(F("GPRS connected"));

  dht.begin();
  traySensor.begin();
  tunnelSensor.begin();
}

void loop() {
  if (!client.connect(SERVER, PORT)) {
    CONSOLE.println(F("Connection to server failed"));
    delay(10000);
    return;
  }

  CONSOLE.println(F("Connected to server"));

  traySensor.requestTemperatures();
  float trayTemp = traySensor.getTempCByIndex(0);
  tunnelSensor.requestTemperatures();
  float tunnelTemp = tunnelSensor.getTempCByIndex(0);
  float ambientTemp = dht.readTemperature();
  float humidity = dht.readHumidity();

  String payload = "{\"tray_temperature\":" + (isnan(trayTemp) ? "null" : String(trayTemp, 2)) +
                   ",\"tunnel_temperature\":" + (isnan(tunnelTemp) ? "null" : String(tunnelTemp, 2)) +
                   ",\"ambient_temperature\":" + (isnan(ambientTemp) ? "null" : String(ambientTemp, 2)) +
                   ",\"humidity\":" + (isnan(humidity) ? "null" : String(humidity, 2)) + "}";

  client.print(payload);
  CONSOLE.println(F("Data sent: "));
  CONSOLE.println(payload);

  client.stop();
  CONSOLE.println(F("Connection closed"));

  for (int i = 0; i < 38; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}
