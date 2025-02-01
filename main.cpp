//=====[Libraries]=============================================================
#include "mbed.h"
#include "arm_book_lib.h"

//=====[Global Objects for Ignition Subsystem]=================================
// Input sensors (assumed to be slider switches)
DigitalIn isDriverSeatOccupied(D4);        // Driver seat occupied
DigitalIn isPassengerSeatOccupied(D5);       // Passenger seat occupied
DigitalIn isDriverSeatbeltFastened(D2);      // Driver seatbelt fastened
DigitalIn isPassengerSeatbeltFastened(D3);   // Passenger seatbelt fastened

// Ignition button (using D6 as in your original code)
DigitalIn isStartEngineButtonPressed(D6);

// Output indicators for ignition
DigitalOut engineReadyLed(LED1);             // Green LED: ignition enabled
DigitalOut engineRunningLed(LED2);           // Blue LED: engine running

// Alarm buzzer (active low when output)
DigitalInOut carAlarmSignal(PE_10);

// UART for serial communication
UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Global Objects for Low Beam Subsystem]===============================
// Potentiometer for low beam mode selector (A0)
AnalogIn headlightPot(A0);

// Light sensor (LDR) for ambient light using averaging; connected to A1.
#define NUMBER_OF_AVG_SAMPLES 10
AnalogIn lightSensorPin(A1);
float ReadingsArray[NUMBER_OF_AVG_SAMPLES]; //average samples for light sensor

//=====[Global Variables]======================================================

// --- Engine State --- (using macros)
#define ENGINE_OFF     0
#define ENGINE_RUNNING 1
#define ENGINE_ALARM   2
int state = ENGINE_OFF; // current engine state

// For ignition button edge detection.
bool prevButtonState = false;

// To ensure the welcome message is printed only once.
bool welcomeDisplayed = false;

// --- Low Beam Modes ---
#define HL_OFF  0
#define HL_AUTO 1
#define HL_ON   2
int headlightMode = HL_OFF;       // current mode from pot
int lastHeadlightMode = -1;       // used to detect mode changes

// We'll simulate left and right low beam lamps with two booleans.
bool leftHeadOn = false;
bool rightHeadOn = false;

// Timer for low beam delays in AUTO mode.
Timer headlightTimer;
bool headlightTimerActive = false;

// Define thresholds.  
// (Because your LDR circuit produces high values when dark, we swap the “ideal” logic.)
#define BRIGHT_THRESHOLD 0.3f   // sensor reading below 0.3 means it is bright
#define DARK_THRESHOLD   0.8f   // sensor reading above 0.8 means it is dark

//=====[Function Prototypes]===================================================
void updateIgnitionEnabledState();
void checkButtonEvent();
bool ignitionConditionsMet();
void printInhibitionReasons();

void lightSensorInit();
float lightSensorUpdate();

void updateHeadlightModeSelection();
void updateHeadlightSubsystem();

//=====[Main Function]=========================================================
int main() {
    // --- Configure ignition input pins with pull-down resistors ---
    isDriverSeatOccupied.mode(PullDown);
    isPassengerSeatOccupied.mode(PullDown);
    isDriverSeatbeltFastened.mode(PullDown);
    isPassengerSeatbeltFastened.mode(PullDown);
    isStartEngineButtonPressed.mode(PullDown);
    
    // --- Configure alarm buzzer ---
    carAlarmSignal.mode(OpenDrain);
    carAlarmSignal.input();
    
    // --- Set initial ignition LED states ---
    engineReadyLed = OFF;
    engineRunningLed = OFF;
    
    // --- Initialize light sensor averaging ---
    lightSensorInit();
    
    // --- Start the low beam timer ---
    headlightTimer.start();
    
    while (true) {
        // ==== Ignition Subsystem ====
        if (state == ENGINE_OFF) {
            if (isDriverSeatOccupied && !welcomeDisplayed) {
                uartUsb.write("Welcome to enhanced alarm system model 218-W25\r\n", 50);
                welcomeDisplayed = true;
            }
            updateIgnitionEnabledState();
        } else {
            engineReadyLed = OFF;
        }
        
        if (state == ENGINE_ALARM) {
            carAlarmSignal.output();
            carAlarmSignal = LOW;
        } else {
            carAlarmSignal.input();
        }
        
        if (state == ENGINE_RUNNING) {
            engineRunningLed = ON;
        }
        
        checkButtonEvent();
        
        // ==== Low Beam Subsystem ====
        updateHeadlightModeSelection();
        if (headlightMode != lastHeadlightMode) {
            if (headlightMode == HL_AUTO)
                uartUsb.write("Low beam mode: AUTO\r\n", 21);
            lastHeadlightMode = headlightMode;
        }
        updateHeadlightSubsystem();
        
        ThisThread::sleep_for(50ms);
    }
    
    return 0;
}

//=====[Ignition Subsystem Functions]=========================================
bool ignitionConditionsMet() {
    return ( isDriverSeatOccupied &&
             isPassengerSeatOccupied &&
             isDriverSeatbeltFastened &&
             isPassengerSeatbeltFastened );
}

void updateIgnitionEnabledState() {
    if (ignitionConditionsMet())
        engineReadyLed = ON;
    else
        engineReadyLed = OFF;
}

void checkButtonEvent() {
    bool currentButtonState = isStartEngineButtonPressed;
    
    if (currentButtonState && !prevButtonState) {
        if (state == ENGINE_OFF) {
            if (ignitionConditionsMet()) {
                state = ENGINE_RUNNING;
                engineRunningLed = ON;
                engineReadyLed = OFF;
                uartUsb.write("Engine started\r\n", 18);
            } else {
                state = ENGINE_ALARM;
                uartUsb.write("Ignition inhibited\r\n", 22);
                printInhibitionReasons();
            }
        } else if (state == ENGINE_RUNNING) {
            state = ENGINE_OFF;
            engineRunningLed = OFF;
            uartUsb.write("Engine stopped\r\n", 18);
            welcomeDisplayed = false;
        } else if (state == ENGINE_ALARM) {
            if (ignitionConditionsMet()) {
                state = ENGINE_RUNNING;
                engineRunningLed = ON;
                uartUsb.write("Engine started\r\n", 18);
            } else {
                uartUsb.write("Ignition inhibited\r\n", 22);
                printInhibitionReasons();
            }
        }
    }
    
    prevButtonState = currentButtonState;
}

void printInhibitionReasons() {
    if (!isDriverSeatOccupied)
        uartUsb.write("Driver seat not occupied\r\n", 28);
    if (!isPassengerSeatOccupied)
        uartUsb.write("Passenger seat not occupied\r\n", 31);
    if (!isDriverSeatbeltFastened)
        uartUsb.write("Driver seatbelt not fastened\r\n", 32);
    if (!isPassengerSeatbeltFastened)
        uartUsb.write("Passenger seatbelt not fastened\r\n", 35);
}

//=====[Light Sensor Functions]===============================================
void lightSensorInit() {
    for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        ReadingsArray[i] = 0;
    }
}

float lightSensorUpdate() {
    static int SampleIndex = 0;
    float readingsSum = 0.0f;
    
    ReadingsArray[SampleIndex] = lightSensorPin.read();
    SampleIndex++;
    if (SampleIndex >= NUMBER_OF_AVG_SAMPLES)
        SampleIndex = 0;
    
    for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        readingsSum += ReadingsArray[i];
    }
    return (readingsSum / NUMBER_OF_AVG_SAMPLES);
}

//=====[Low Beam Subsystem Functions]=========================================
void updateHeadlightModeSelection() {
    float potValue = headlightPot.read();
    if (potValue < 0.33f)
        headlightMode = HL_OFF;
    else if (potValue < 0.66f)
        headlightMode = HL_AUTO;
    else
        headlightMode = HL_ON;
}

void updateHeadlightSubsystem() {
    if (state != ENGINE_RUNNING) {
        if (leftHeadOn || rightHeadOn) {
            uartUsb.write("Both low beam lamps turned OFF\r\n", 32);
            leftHeadOn = false;
            rightHeadOn = false;
        }
        headlightTimerActive = false;
        return;
    }
    
    if (headlightMode == HL_OFF) {
        if (leftHeadOn || rightHeadOn) {
            uartUsb.write("Both low beam lamps turned OFF\r\n", 32);
            leftHeadOn = false;
            rightHeadOn = false;
        }
        headlightTimerActive = false;
    } else if (headlightMode == HL_ON) {
        if (!leftHeadOn || !rightHeadOn) {
            uartUsb.write("Both low beam lamps turned ON\r\n", 31);
            leftHeadOn = true;
            rightHeadOn = true;
        }
        headlightTimerActive = false;
    } else if (headlightMode == HL_AUTO) {
        float lightLevel = lightSensorUpdate();
        if (lightLevel > DARK_THRESHOLD) {
            if (!leftHeadOn || !rightHeadOn) {
                if (!headlightTimerActive) {
                    headlightTimer.reset();
                    headlightTimerActive = true;
                } else if (headlightTimer.read() >= 1.0f) {
                    uartUsb.write("Both low beam lamps turned ON\r\n", 31);
                    leftHeadOn = true;
                    rightHeadOn = true;
                    headlightTimerActive = false;
                }
            } else {
                headlightTimerActive = false;
            }
        } else if (lightLevel < BRIGHT_THRESHOLD) {
            if (leftHeadOn || rightHeadOn) {
                if (!headlightTimerActive) {
                    headlightTimer.reset();
                    headlightTimerActive = true;
                } else if (headlightTimer.read() >= 2.0f) {
                    uartUsb.write("Both low beam lamps turned OFF\r\n", 32);
                    leftHeadOn = false;
                    rightHeadOn = false;
                    headlightTimerActive = false;
                }
            } else {
                headlightTimerActive = false;
            }
        } else {
            headlightTimerActive = false;
        }
    }
}
