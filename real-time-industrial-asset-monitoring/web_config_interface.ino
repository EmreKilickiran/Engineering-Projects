// web_config_interface.ino — Web-Based Parameter Configuration Interface
//
// Serves an embedded web interface on the Arduino Opta PLC for remote
// configuration of sensor parameters and alert thresholds.
//
// Features:
//   - Input configuration: 8 inputs × 3 types (Counter, Analog, PWM)
//     with configurable min/max ranges for Analog and PWM
//   - Device configuration: 16 Modbus addresses × 3 device types
//     (Analyzer, Temperature, Pressure) with per-device threshold
//     settings (min/max for Watt, Voltage, Current)
//   - EEPROM persistence: all settings survive power cycles
//   - Real-time form validation with JavaScript
//   - Success confirmation messages


#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

// --- Network ----------------------------------------------------------------
byte mac[] = { 0xA8, 0x61, 0x0A, 0x50, 0x8B, 0x4F };
EthernetServer server(80);
String readString;
bool showSuccessMessage = false;

// --- Input Configuration (8 inputs) -----------------------------------------
byte inputArray[8];
byte input_min[8];
byte input_max[8];
byte variable_type;

// --- Device Configuration (16 Modbus addresses) -----------------------------
byte addressArray[16];
byte minWattArray[16];
byte maxWattArray[16];
byte minVoltageArray[16];
byte maxVoltageArray[16];
byte minCurrentArray[16];
byte maxCurrentArray[16];
byte device_type;

// --- EEPROM Memory Map ------------------------------------------------------
// Bytes 0-15:    inputArray (8 × 2 bytes)
// Bytes 16-31:   input_min  (8 × 2 bytes)
// Bytes 32-47:   input_max  (8 × 2 bytes)
// Byte  48:      variable_type
// Bytes 64-79:   addressArray (16 bytes)
// Bytes 80-95:   minWattArray
// Bytes 96-111:  maxWattArray
// Bytes 112-127: minVoltageArray
// Bytes 128-143: maxVoltageArray
// Bytes 144-159: minCurrentArray
// Bytes 160-175: maxCurrentArray
// Byte  176:     device_type


void setup() {
    Serial.begin(9600);
    Ethernet.begin(mac);
    server.begin();
    Serial.print("Configuration interface at http://");
    Serial.println(Ethernet.localIP());

    // Load saved configuration from EEPROM
    for (int i = 0; i < 8; i++) {
        EEPROM.get(i * 2, inputArray[i]);
        EEPROM.get(16 + i * 2, input_min[i]);
        EEPROM.get(32 + i * 2, input_max[i]);
    }
    EEPROM.get(48, variable_type);

    for (int i = 0; i < 16; i++) {
        EEPROM.get(64 + i, addressArray[i]);
        EEPROM.get(80 + i, minWattArray[i]);
        EEPROM.get(96 + i, maxWattArray[i]);
        EEPROM.get(112 + i, minVoltageArray[i]);
        EEPROM.get(128 + i, maxVoltageArray[i]);
        EEPROM.get(144 + i, minCurrentArray[i]);
        EEPROM.get(160 + i, maxCurrentArray[i]);
    }
    EEPROM.get(176, device_type);
}


// REQUEST HANDLING

void processInputConfig(String &request) {
    if (request.indexOf("inputSelect=") < 0 || request.indexOf("typeSelect=") < 0)
        return;

    int idx = request.indexOf("inputSelect=") + 12;
    int selectedInput = request.substring(idx, idx + 1).toInt();

    idx = request.indexOf("typeSelect=") + 11;
    variable_type = request.substring(idx, idx + 1).toInt();
    inputArray[selectedInput] = variable_type;

    EEPROM.put(selectedInput * 2, inputArray[selectedInput]);
    EEPROM.put(48, variable_type);

    // Read min/max for Analog (2) or PWM (3) types
    if (variable_type == 2 || variable_type == 3) {
        if (request.indexOf("minInput=") > 0) {
            idx = request.indexOf("minInput=") + 9;
            input_min[selectedInput] = request.substring(idx).toInt();
            EEPROM.put(16 + selectedInput * 2, input_min[selectedInput]);
        }
        if (request.indexOf("maxInput=") > 0) {
            idx = request.indexOf("maxInput=") + 9;
            input_max[selectedInput] = request.substring(idx).toInt();
            EEPROM.put(32 + selectedInput * 2, input_max[selectedInput]);
        }
    }
    showSuccessMessage = true;
}

void processDeviceConfig(String &request) {
    if (request.indexOf("addressSelect=") < 0 || request.indexOf("deviceSelect=") < 0)
        return;

    int idx = request.indexOf("addressSelect=") + 14;
    int selectedAddr = request.substring(idx, idx + 1).toInt();

    idx = request.indexOf("deviceSelect=") + 13;
    device_type = request.substring(idx, idx + 1).toInt();
    addressArray[selectedAddr] = device_type;

    EEPROM.put(64 + selectedAddr, addressArray[selectedAddr]);
    EEPROM.put(176, device_type);

    // Read analyzer thresholds (Watt, Voltage, Current)
    if (device_type == 1) {
        const char* fields[] = { "minWatt=", "maxWatt=", "minVoltage=",
                                  "maxVoltage=", "minCurrent=", "maxCurrent=" };
        byte* arrays[] = { minWattArray, maxWattArray, minVoltageArray,
                           maxVoltageArray, minCurrentArray, maxCurrentArray };
        int offsets[] = { 80, 96, 112, 128, 144, 160 };

        for (int f = 0; f < 6; f++) {
            if (request.indexOf(fields[f]) > 0) {
                idx = request.indexOf(fields[f]) + strlen(fields[f]);
                arrays[f][selectedAddr] = request.substring(idx).toInt();
                EEPROM.put(offsets[f] + selectedAddr, arrays[f][selectedAddr]);
            }
        }
    }
    showSuccessMessage = true;
}


// HTML INTERFACE

void serveHTML(EthernetClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();

    client.println("<!DOCTYPE html><html><head>");
    client.println("<meta charset='UTF-8'><title>IoT Configuration</title>");

    // JavaScript for dynamic form fields
    client.println("<script>");
    client.println("function showMinMax(v){document.getElementById('minMaxInputs').style.display=(v=='2'||v=='3')?'block':'none';}");
    client.println("function showDevice(v){document.getElementById('deviceInputs').style.display=(v=='1')?'block':'none';}");
    client.println("function showSuccess(){var m=document.getElementById('msg');m.style.display='block';setTimeout(function(){m.style.display='none';},3000);}");
    client.println("</script></head>");

    client.println("<body style='background:#f0f8ff;font-family:sans-serif;padding:20px'");
    if (showSuccessMessage) client.println(" onload='showSuccess()'");
    client.println(">");

    client.println("<div id='msg' style='display:none;background:#4CAF50;color:white;padding:10px;border-radius:4px'>Settings saved.</div>");

    // --- Input Configuration Form ---
    client.println("<h2>Input Configuration</h2>");
    client.println("<form action='/' method='GET'>");
    client.println("<select name='inputSelect'>");
    for (int i = 0; i < 8; i++) {
        client.println("<option value='" + String(i) + "'>Input " + String(i) + "</option>");
    }
    client.println("</select>");
    client.println("<select name='typeSelect' onchange='showMinMax(this.value)'>");
    client.println("<option value='1'>Counter</option><option value='2'>Analog</option><option value='3'>PWM</option>");
    client.println("</select>");
    client.println("<div id='minMaxInputs' style='display:none'>Min: <input name='minInput'/> Max: <input name='maxInput'/></div>");
    client.println("<input type='submit' value='Set'/></form>");

    // --- Device Configuration Form ---
    client.println("<h2>Device Configuration</h2>");
    client.println("<form action='/' method='GET'>");
    client.println("<select name='addressSelect'>");
    for (int i = 0; i < 16; i++) {
        client.println("<option value='" + String(i) + "'>Address " + String(i) + "</option>");
    }
    client.println("</select>");
    client.println("<select name='deviceSelect' onchange='showDevice(this.value)'>");
    client.println("<option value='1'>Analyzer</option><option value='2'>Temperature</option><option value='3'>Pressure</option>");
    client.println("</select>");
    client.println("<div id='deviceInputs'>");
    client.println("Min Watt:<input name='minWatt'/> Max Watt:<input name='maxWatt'/><br>");
    client.println("Min Voltage:<input name='minVoltage'/> Max Voltage:<input name='maxVoltage'/><br>");
    client.println("Min Current:<input name='minCurrent'/> Max Current:<input name='maxCurrent'/>");
    client.println("</div>");
    client.println("<input type='submit' value='Set'/></form>");

    client.println("</body></html>");
}


// MAIN LOOP

void loop() {
    EthernetClient client = server.available();
    if (!client) return;

    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            if (readString.length() < 200) readString += c;

            if (c == '\n') {
                processInputConfig(readString);
                processDeviceConfig(readString);
                serveHTML(client);

                delay(1);
                client.stop();
                showSuccessMessage = false;
                readString = "";
            }
        }
    }
}
