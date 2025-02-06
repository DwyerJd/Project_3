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
DigitalIn isStartEngineButtonPressed(BUTTON1);

// Output indicators for ignition
DigitalOut engineReadyLed(LED1);             // Green LED: ignition enabled
DigitalOut engineRunningLed(LED2);           // Blue LED: engine running

// Alarm buzzer (active low when output)
DigitalInOut carAlarmSignal(PE_10);

// UART for serial communication
UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

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

void 