System Description

This project implements an integrated automotive ignition and low beam headlight control system using an embedded platform. 
The ignition subsystem verifies that both the driver and passenger seats are occupied and that both seatbelts are fastened before enabling engine start (indicated by a green LED). 
Once enabled, pressing the ignition button starts the engine (blue LED on) or, if conditions are not met, displays appropriate error messages on the serial monitor. 
The low beam subsystem is controlled by a potentiometer and an ambient light sensor. 
In AUTO mode, the system uses the sensor reading to determine if the low beam lamps should be turned on or off after a specified delay; note that there are no physical headlight LEDs—this behavior is displayed via messages on the serial monitor. 
The system is designed to maintain engine and low beam states until the appropriate user input changes the state.

Starting Repository
N/A

Summary of Testing Results
Ignition Subsystem

Specification: Enable engine start (i.e., light the green LED) when both seats are occupied and seatbelts fastened; otherwise, display appropriate error messages.
Test Result: Pass
Comment: All error messages are displayed on the serial monitor.

Specification: Start the engine (i.e., light the blue LED, turn off green LED) when ignition is enabled and the ignition button is pressed before release.
Test Result: Pass
Comment: Engine starts reliably when conditions are met.

Specification: Keep the engine running even if the driver/passenger unfasten belt or exit the vehicle.
Test Result: Pass
Comment: The engine remains running once started.

Specification: When the engine is running, stop the engine only after the ignition button has been pushed and then released.
Test Result: Pass
Comment:

Low Beam Subsystem

Specification: In HL_ON mode, force both low beam lamps ON and display the corresponding message on the serial monitor.
Test Result: Pass
Comment: Both low beam lamps are reported as turned ON via the serial monitor.

Specification: In HL_OFF mode, force both low beam lamps OFF and display the corresponding message on the serial monitor.
Test Result: Pass
Comment: Both low beam lamps are reported as turned OFF via the serial monitor.

Specification: In HL_AUTO mode, automatically turn the low beam lamps ON (after a 1-second delay) when the ambient light sensor reading is above the DARK threshold (indicating darkness).
Test Result: Pass
Comment: Low beam lamps are reported as turned ON in dark conditions after the specified delay.

Specification: In HL_AUTO mode, automatically turn the low beam lamps OFF (after a 2-second delay) when the ambient light sensor reading is below the BRIGHT threshold (indicating brightness).
Test Result: Pass
Comment: Low beam lamps are reported as turned OFF in bright conditions after the specified delay.

Specification: Note: There are no physical low beam LEDs; the system displays the corresponding low beam state on the serial monitor.
Test Result: N/A
Comment: All low beam outputs are simulated via serial monitor messages.
