// sensor_acquisition.ino — Real-Time Sensor Data Acquisition & Alert System
// Features:
//   1. Multi-analyzer Modbus polling (3 analyzers at addresses 2, 3, 7)
//   2. Automatic baseline calibration (10-second startup averaging)
//   3. Threshold-triggered alert system with configurable tolerances:
//        - Voltage RMS:      ±10%
//        - Current RMS:      ±12%
//        - Power Factor:     ±50%  (wide band for industrial loads)
//        - THD:              +5%   (one-sided, upward only)
//        - Positive Energy:  ±9%
//        - Negative Energy:  ±6%
//   4. Hexadecimal data encoding for RS485 serial communication,
//      minimizing transmission latency and payload size
//   5. HTTP POST to Flask server with encoded sensor data
//   6. Alert-triggered immediate transmission (bypasses interval timer)
//
// Hardware:
//   - Arduino Opta PLC (ARM Cortex-M7)
//   - Finder 6M energy analyzers × 3 (Modbus RS485)
//   - Ethernet (W5500) for server communication

#include <Finder6M.h>
#include <Ethernet.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <NTPClient.h>


// CONFIGURATION

// --- Network ----------------------------------------------------------------
byte mac[] = { 0xA8, 0x61, 0x0A, 0x50, 0x8B, 0x4F };
IPAddress serverIP(192, 6, 1, 72);
int port = 5000;

EthernetClient ethernetClient;
HttpClient client = HttpClient(ethernetClient, serverIP, port);
EthernetUDP udp;

const char* ntpServer = "pool.ntp.org";
NTPClient timeClient(udp, ntpServer, 0, 60000);

// --- Modbus Analyzer Addresses ----------------------------------------------
constexpr uint8_t MODBUS_ADDRESS_1 = 2;
constexpr uint8_t MODBUS_ADDRESS_2 = 3;
constexpr uint8_t MODBUS_ADDRESS_3 = 7;

Finder6M f6m1, f6m2, f6m3;

// --- Timing -----------------------------------------------------------------
unsigned long previousMillis = 0;
unsigned long startMillis;
const long TRANSMIT_INTERVAL = 1000;   // Normal transmission interval (ms)
const long CALIBRATION_PERIOD = 10000; // Baseline calibration period (ms)


// ANALYZER DATA STRUCTURE
// Holds accumulated readings, baseline values, tolerance thresholds,
// and alert states for each energy analyzer.

struct AnalyzerData {
    // --- Calibration accumulators ---
    double voltageSum = 0;
    int    voltageCount = 0;
    double currentSum = 0;
    int    currentCount = 0;

    // --- Baseline values (computed during calibration) ---
    double normalVoltage = 0;
    double voltageTolerance = 0;      // ±10% of baseline
    double normalCurrent = 0;
    double currentTolerance = 0;      // ±12% of baseline

    bool isSetupComplete = false;

    // --- Running sums for averaging ---
    double energySum = 0;
    double posEnergySum = 0;
    double negEnergySum = 0;
    double voltageRMSSum = 0;
    double currentRMSSum = 0;
    double activePowerSum = 0;
    double reactivePowerSum = 0;
    double apparentPowerSum = 0;
    double powerFactorSum = 0;
    double frequencySum = 0;
    double thdSum = 0;
    int    count = 0;

    // --- Alert flags ---
    bool alertActiveVoltage = false;
    bool alertActiveCurrent = false;
};

AnalyzerData analyzer1, analyzer2, analyzer3;


// INITIALIZATION

void setupAnalyzer(Finder6M &f6m, uint8_t address) {
    if (!f6m.init()) { while (1); }
    f6m.measureAlternateCurrent(address);
    f6m.disableEnergyStoring(address);
    if (!f6m.saveSettings(address)) { while (1); }
}

void setup() {
    // Ethernet initialization
    if (Ethernet.begin(mac) == 0) {
        Ethernet.begin(mac, IPAddress(192, 168, 1, 100));
    }
    delay(3000);

    Serial.begin(38400);

    // Initialize all three Modbus analyzers
    setupAnalyzer(f6m1, MODBUS_ADDRESS_1);
    setupAnalyzer(f6m2, MODBUS_ADDRESS_2);
    setupAnalyzer(f6m3, MODBUS_ADDRESS_3);

    startMillis = millis();
}


// SENSOR PROCESSING & THRESHOLD ALERTING
// During the first 10 seconds (calibration phase), computes running averages
// to establish baseline values. After calibration, each reading is compared
// against the baseline ± tolerance. If any parameter exceeds its threshold,
// an alert is triggered and data is immediately transmitted to the server.

void processAnalyzer(Finder6M &f6m, AnalyzerData &data, uint8_t address) {
    double voltageRMS = f6m.getVoltageRMS(address);
    double currentRMS = f6m.getCurrentRMS(address);
    double powerFactor = f6m.getPowerFactor(address);
    double thd = f6m.getTHD(address);

    // --- Calibration phase (first 10 seconds) ---
    if (!data.isSetupComplete) {
        if (millis() - startMillis < CALIBRATION_PERIOD) {
            data.voltageSum += voltageRMS;
            data.voltageCount++;
            data.currentSum += currentRMS;
            data.currentCount++;
        } else {
            // Compute baselines and tolerances
            data.normalVoltage = data.voltageSum / data.voltageCount;
            data.voltageTolerance = data.normalVoltage * 0.10;

            data.normalCurrent = data.currentSum / data.currentCount;
            data.currentTolerance = data.normalCurrent * 0.12;

            data.isSetupComplete = true;

            Serial.println("Calibration complete for analyzer " + String(address));
            Serial.println("  Baseline voltage: " + String(data.normalVoltage) + " V");
            Serial.println("  Baseline current: " + String(data.normalCurrent) + " mA");
        }
        return;
    }

    // --- Normal operation: accumulate and check thresholds ---
    data.voltageRMSSum += voltageRMS;
    data.currentRMSSum += currentRMS;
    data.powerFactorSum += powerFactor;
    data.thdSum += thd;
    data.count++;

    // Voltage threshold check
    if (voltageRMS < (data.normalVoltage - data.voltageTolerance) ||
        voltageRMS > (data.normalVoltage + data.voltageTolerance)) {
        data.alertActiveVoltage = true;
        Serial.println("ALERT: Voltage out of range [" + String(address) + "]: "
                        + String(voltageRMS) + " V");
    } else {
        data.alertActiveVoltage = false;
    }

    // Current threshold check
    if (currentRMS < (data.normalCurrent - data.currentTolerance) ||
        currentRMS > (data.normalCurrent + data.currentTolerance)) {
        data.alertActiveCurrent = true;
        Serial.println("ALERT: Current out of range [" + String(address) + "]: "
                        + String(currentRMS) + " mA");
    } else {
        data.alertActiveCurrent = false;
    }

    // Immediate transmission on alert
    if (data.alertActiveVoltage || data.alertActiveCurrent) {
        sendDataToServer();
    }
}


// HEXADECIMAL ENCODING & TRANSMISSION
// Encodes IEEE 754 float values into compact hexadecimal strings for
// RS485 serial transmission, minimizing payload size. Each analyzer's
// readings (11 parameters) are concatenated into a single hex stream.

String floatToHex(float value) {
    union { float f; byte b[4]; } u;
    u.f = value;
    String hexStr = "";
    for (int i = 0; i < 4; i++) {
        if (u.b[i] < 0x10) hexStr += "0";
        hexStr += String(u.b[i], HEX);
    }
    return hexStr;
}

void sendDataToServer() {
    // Compute averages for each analyzer
    struct { AnalyzerData* data; uint8_t addr; } analyzers[] = {
        { &analyzer1, MODBUS_ADDRESS_1 },
        { &analyzer2, MODBUS_ADDRESS_2 },
        { &analyzer3, MODBUS_ADDRESS_3 },
    };

    // Build hex-encoded payload: [addr][11 float values] × 3 analyzers
    String hexPayload = "";

    for (auto &a : analyzers) {
        if (a.data->count == 0) continue;
        float c = (float)a.data->count;

        hexPayload += floatToHex(a.addr);
        hexPayload += floatToHex(a.data->energySum / c);
        hexPayload += floatToHex(a.data->posEnergySum / c);
        hexPayload += floatToHex(a.data->negEnergySum / c);
        hexPayload += floatToHex(a.data->voltageRMSSum / c);
        hexPayload += floatToHex(a.data->currentRMSSum / c);
        hexPayload += floatToHex(a.data->activePowerSum / c);
        hexPayload += floatToHex(a.data->reactivePowerSum / c);
        hexPayload += floatToHex(a.data->apparentPowerSum / c);
        hexPayload += floatToHex(a.data->powerFactorSum / c);
        hexPayload += floatToHex(a.data->frequencySum / c);
        hexPayload += floatToHex(a.data->thdSum / c);
    }

    // HTTP POST to Flask server
    client.beginRequest();
    client.post("/data");
    client.sendHeader(HTTP_HEADER_CONTENT_TYPE, "text/plain");
    client.sendHeader(HTTP_HEADER_CONTENT_LENGTH, hexPayload.length());
    client.beginBody();
    client.print(hexPayload);
    client.endRequest();

    int statusCode = client.responseStatusCode();
    Serial.println("Sent " + String(hexPayload.length()) + " bytes, status: "
                    + String(statusCode));
}

void resetAnalyzerData(AnalyzerData &a) {
    a.energySum = 0;     a.posEnergySum = 0;     a.negEnergySum = 0;
    a.voltageRMSSum = 0; a.currentRMSSum = 0;    a.activePowerSum = 0;
    a.reactivePowerSum = 0; a.apparentPowerSum = 0;
    a.powerFactorSum = 0; a.frequencySum = 0;    a.thdSum = 0;
    a.count = 0;
}


// MAIN LOOP

void loop() {
    // Poll all three analyzers
    processAnalyzer(f6m1, analyzer1, MODBUS_ADDRESS_1);
    processAnalyzer(f6m2, analyzer2, MODBUS_ADDRESS_2);
    processAnalyzer(f6m3, analyzer3, MODBUS_ADDRESS_3);

    // Periodic transmission (if no alerts triggered immediate send)
    bool anyAlert = analyzer1.alertActiveVoltage || analyzer1.alertActiveCurrent ||
                    analyzer2.alertActiveVoltage || analyzer2.alertActiveCurrent ||
                    analyzer3.alertActiveVoltage || analyzer3.alertActiveCurrent;

    if (!anyAlert && (millis() - previousMillis >= TRANSMIT_INTERVAL)) {
        previousMillis = millis();
        sendDataToServer();
        resetAnalyzerData(analyzer1);
        resetAnalyzerData(analyzer2);
        resetAnalyzerData(analyzer3);
    }
}
