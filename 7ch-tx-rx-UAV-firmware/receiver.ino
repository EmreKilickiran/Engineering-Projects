// receiver.ino — 6-Channel Wireless RC Receiver with Failsafe
//
// Receives control packets from the transmitter via NRF24L01+, converts
// each channel to standard RC PWM signals (1000-2000 µs), and drives
// servo motors on pins D2-D7.
//
// Failsafe: If no packet is received for 1 second, all channels reset
// to safe defaults (throttle off, surfaces centered).
//
// PWM Output Mapping:
//   D2 → Roll       D5 → Yaw
//   D3 → Pitch      D6 → Aux1
//   D4 → Throttle   D7 → Aux2
//
// Hardware: Arduino Nano + NRF24L01+ (CE=9, CSN=10) + 6× Servo


#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>

// --- Radio Configuration ----------------------------------------------------
const uint64_t PIPE_ADDRESS = 0xE9E8F0F0E1LL;  // Must match transmitter
RF24 radio(9, 10);

// --- Failsafe timeout -------------------------------------------------------
const unsigned long FAILSAFE_TIMEOUT = 1000;  // ms
unsigned long lastRecvTime = 0;

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

// --- Servo Channels ---------------------------------------------------------
Servo ch1, ch2, ch3, ch4, ch5, ch6;

// --- Safe Defaults ----------------------------------------------------------
void ResetData() {
    data.roll     = 127;
    data.pitch    = 127;
    data.throttle = 12;    // Motor off
    data.yaw      = 127;
    data.aux1     = 127;
    data.aux2     = 127;
}


void setup() {
    // Attach servos to PWM output pins
    ch1.attach(2);   // Roll
    ch2.attach(3);   // Pitch
    ch3.attach(4);   // Throttle
    ch4.attach(5);   // Yaw
    ch5.attach(6);   // Aux1
    ch6.attach(7);   // Aux2

    ResetData();
    radio.begin();
    radio.openReadingPipe(1, PIPE_ADDRESS);
    radio.setAutoAck(false);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_HIGH);
    radio.startListening();
}

void recvData() {
    while (radio.available()) {
        radio.read(&data, sizeof(Signal));
        lastRecvTime = millis();
    }
}

void loop() {
    recvData();

    // Failsafe: reset if no signal for 1 second
    if (millis() - lastRecvTime > FAILSAFE_TIMEOUT) {
        ResetData();
    }

    // Convert 0-255 data to standard RC PWM range (1000-2000 µs)
    ch1.writeMicroseconds(map(data.roll,     0, 255, 1000, 2000));
    ch2.writeMicroseconds(map(data.pitch,    0, 255, 1000, 2000));
    ch3.writeMicroseconds(map(data.throttle, 0, 255, 1000, 2000));
    ch4.writeMicroseconds(map(data.yaw,      0, 255, 1000, 2000));
    ch5.writeMicroseconds(map(data.aux1,     0, 255, 1000, 2000));
    ch6.writeMicroseconds(map(data.aux2,     0, 255, 1000, 2000));
}
