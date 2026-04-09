// ============================================================================
// timestamp_encoder.ino — NTP Timestamp Acquisition & Hex Encoding
// ============================================================================
//
// Acquires real-time timestamps via NTP (pool.ntp.org), encodes date and time
// into compact hexadecimal format, and transmits as JSON to the Flask server.
//
// Encoding format:
//   Date: DDMMYY → hex (e.g., 140824 → 0x22628)
//   Time: HHMMSS → hex (e.g., 144245 → 0x23375)
//   Combined: date_hex + time_hex as a single string
//
// This compact encoding reduces payload size for RS485 serial transmission.
//
// Author : Yunus Emre Kılıçkıran
// ============================================================================

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

// --- Network Configuration --------------------------------------------------
byte mac[] = { 0xA8, 0x61, 0x0A, 0x50, 0x8B, 0x4F };
IPAddress serverIP(192, 6, 1, 72);
int port = 5000;

EthernetClient ethernetClient;
HttpClient client = HttpClient(ethernetClient, serverIP, port);
EthernetUDP udp;

// --- NTP Configuration (GMT+3 for Turkey) -----------------------------------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3 * 3600;

NTPClient timeClient(udp, ntpServer, gmtOffset_sec, 60000);

// ============================================================================

void setup() {
    Serial.begin(115200);

    if (Ethernet.begin(mac) == 0) {
        Ethernet.begin(mac, IPAddress(192, 6, 1, 72));
    }
    delay(1000);
    Serial.println("Ethernet connected.");

    timeClient.begin();
}

void loop() {
    timeClient.update();

    unsigned long epochTime = timeClient.getEpochTime();
    time_t localTime = epochTime;
    struct tm *ptm = gmtime(&localTime);

    // --- Date encoding: DDMMYY → single integer → hex ---
    int day   = ptm->tm_mday;
    int month = ptm->tm_mon + 1;
    int year  = (ptm->tm_year + 1900) % 100;  // Last two digits

    int formattedDate = day * 10000 + month * 100 + year;
    char hexDate[7];
    sprintf(hexDate, "%X", formattedDate);

    // --- Time encoding: HHMMSS → single integer → hex ---
    int hour   = ptm->tm_hour;
    int minute = ptm->tm_min;
    int second = ptm->tm_sec;

    int formattedTime = hour * 10000 + minute * 100 + second;
    char hexTime[7];
    sprintf(hexTime, "%X", formattedTime);

    // --- Combine into single timestamp string ---
    char combinedDateTime[15];
    sprintf(combinedDateTime, "%s%s", hexDate, hexTime);

    Serial.print("Hex timestamp: ");
    Serial.println(combinedDateTime);

    // --- Transmit as JSON ---
    StaticJsonDocument<200> doc;
    doc["datetime"] = combinedDateTime;

    String output;
    serializeJson(doc, output);

    client.beginRequest();
    client.post("/data");
    client.sendHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
    client.sendHeader(HTTP_HEADER_CONTENT_LENGTH, output.length());
    client.beginBody();
    client.print(output);
    client.endRequest();

    delay(1000);
}
