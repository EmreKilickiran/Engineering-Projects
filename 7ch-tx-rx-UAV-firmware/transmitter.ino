// ============================================================================
// transmitter.ino — 6-Channel Wireless RC Transmitter
// ============================================================================
//
// Reads 6 analog inputs (4 joystick axes + 2 auxiliary potentiometers),
// maps them to calibrated 0-255 PWM range with configurable direction,
// and transmits the control packet at 250 kbps via NRF24L01+ radio.
//
// Signal lost behavior: all channels reset to safe defaults (throttle off,
// control surfaces centered).
//
// Hardware: Arduino Nano + NRF24L01+ (CE=9, CSN=10)
// Channels: Throttle (A0), Yaw (A1), Pitch (A2), Roll (A3), Aux1 (A6), Aux2 (A7)
//
// Author : Yunus Emre Kılıçkıran
// ============================================================================

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- Radio Configuration ----------------------------------------------------
const uint64_t PIPE_ADDRESS = 0xE9E8F0F0E1LL;  // Must match receiver
RF24 radio(9, 10);                               // CE, CSN pins

// --- Control Signal Packet --------------------------------------------------
struct Signal {
    byte throttle;
    byte pitch;
    byte roll;
    byte yaw;
    byte aux1;
    byte aux2;
};

Signal data;

// --- Safe Defaults (signal-lost position) -----------------------------------
void ResetData() {
    data.throttle = 12;    // Motor off
    data.pitch    = 127;   // Center
    data.roll     = 127;   // Center
    data.yaw      = 127;   // Center
    data.aux1     = 127;   // Center
    data.aux2     = 127;   // Center
}

// ============================================================================
// Joystick Calibration
// ============================================================================
// Maps raw ADC values (0-1023) to 0-255 with dead-zone handling around
// the mechanical center point. Supports direction reversal per channel.

int mapJoystickValues(int val, int lower, int middle, int upper, bool reverse) {
    val = constrain(val, lower, upper);
    if (val < middle)
        val = map(val, lower, middle, 0, 128);
    else
        val = map(val, middle, upper, 128, 255);
    return reverse ? 255 - val : val;
}

// ============================================================================

void setup() {
    radio.begin();
    radio.openWritingPipe(PIPE_ADDRESS);
    radio.setAutoAck(false);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_HIGH);
    radio.stopListening();
    ResetData();
}

void loop() {
    // Read and calibrate each control channel
    // Adjust lower/middle/upper values per your joystick hardware
    data.throttle = mapJoystickValues(analogRead(A0), 12, 524, 1020, true);
    data.roll     = mapJoystickValues(analogRead(A3), 12, 524, 1020, true);
    data.pitch    = mapJoystickValues(analogRead(A2), 12, 524, 1020, false);
    data.yaw      = mapJoystickValues(analogRead(A1), 12, 524, 1020, false);
    data.aux1     = mapJoystickValues(analogRead(A6), 12, 524, 1020, true);
    data.aux2     = mapJoystickValues(analogRead(A7), 12, 524, 1020, true);

    radio.write(&data, sizeof(Signal));
}
