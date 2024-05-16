/////////////////////////////////////////////////////////////////////////////////
// GenericlockBoard.h
//
// Contains the GenericClockBoard class.  This class declares the base interface
// to the Generic Clock Board.  See %%%jmc
//
// It is meant to control, and has been tested with, a 28BYJ-48 stepper motor.
// Other stepper motors may work, but some constants may need to be changed.
//
// Note that this implementation relies heavily on the use of an ESP32.  Most
// ESP32 boards should work with the code.  However, the code probably won't
// work with other processors, including the ESP8266.
//
// Another caveat is that this implementation requires that the ports used
// to drive the stepper must all reside in first 32 GPIO pins of the ESP32.
// That is, only GPIO 0 through GPIO 31 may be used to run the stepper.
//
// This implementation relies on the Arduino SerialDebug library to support
// debugging and status information via Serial I/O.  See
// https://randomnerdtutorials.com/software-debugger-arduino-ide-serialdebug-library/
// and  https://github.com/JoaoLopesF/SerialDebug for more information.
// It also uses the Arduino RGBLedlibrary to support dimmable RGB LEDs.  More
// information regarding this library can be found at
// https://github.com/wilmouths/RGBLed.
//
// History:
//  - jmcorbett 11-MAY-2024
//    Original creation.
//
// Copyright (c) 2024, Joseph M. Corbett
//
/////////////////////////////////////////////////////////////////////////////////
#if !defined GENERICCLOCKBOARD_H
#define GENERICCLOCKBOARD_H

#include "SerialDebugSetup.h"   // For common SerialDebug options.
#include <RGBLed.h>             // For RGBLed class supports the board's RGB LEDs.


/////////////////////////////////////////////////////////////////////////////////
// StepperSpeed_t
//
// This enum is used to select the speed that will be used for stepper movement
// via the Step() method.  The selections are:
//      StepSlow - Will move the stepper at slow speed for the full duration of
//                 the move.
//      StepAuto - Will make long moves use the fast speed with accel and decel
//                 at the beginning and end of the move.  Short moves will be
//                 done at slow speed.
//      StepFast - Will move the stepper at fast speed for the full duration of
//                 the move.
/////////////////////////////////////////////////////////////////////////////////
enum StepperSpeed_t
{
    StepSlow = -1,      // Select slow stepper speed.
    StepAuto =  0,      // Select automatic stepper speed with accel and decel.
    StepFast =  1       // Select fast stepper speed.
};



/////////////////////////////////////////////////////////////////////////////////
// GenericClockBoard class
//
// Contains I/O pin definitions and supporting methods to interface to them.
/////////////////////////////////////////////////////////////////////////////////
class GenericClockBoard
{
public:
    /////////////////////////////////////////////////////////////////////////////
    // GenericClockBoard()  (constructor)
    //
    // Constructs a class instance, initializes board hardware, and initializes
    // instance variables.
    //
    // Arguments:
    //   - rapidSecondsPerRev  - Specifies the number of seconds it takes the
    //                           stepper to make one full revolution of its outupt
    //                           shaft.  For the 28BYJ-48 stepper motor, a good
    //                           range is normally between 6 and 10 seconds.
    //   - fullStepsPerRev     - Specifies the number of FULL steps per revolution
    //                           of the stepper motor's output shaft.  For the
    //                           28BYJ-48 the value is 2048.
    //   - stepperPinsReversed - Specifies the whether or not the stepper turns
    //                           clockwise when a positive step value is commanded.
    //                           Set to 'true' if a positive step value causes
    //                           counterclockwise movement.  Set to 'false'
    //                           otherwise.
    //   - stepperHalfStepping - Specifies whether half stepping is to be used.
    //                           If 'true', then half stepping is used, which
    //                           will cause the number of steps per rev of the
    //                           stepper to double.  For example, the 28BYJ-48
    //                           stepper will take 4096 steps per rev if this
    //                           value is set to 'true'.  In most cases, use of
    //                           half stepping is a good choice.
    //   - homeNormallyOpen    - Specifies the type of sensor used for homing the
    //                           clock.  Set to 'true' for normally open (N.O.)
    //                           sensors.  Set to 'false' for normally closed
    //                           (N.C.) sensors.
    /////////////////////////////////////////////////////////////////////////////
    GenericClockBoard(
                      uint32_t rapidSecondsPerRev,
                      uint32_t fullStepsPerRev     = 2048,
                      bool     stepperPinsReversed = false,
                      bool     stepperHalfStepping = true,
                      bool     homeNormallyOpen    = true
                      );

    // Destructorl
    ~GenericClockBoard() {}

    /////////////////////////////////////////////////////////////////////////////
    // Step()
    //
    // Step the stepper motor a specific number of steps in a specified direction
    // at a specified speed.
    //
    // Arguments:
    //   steps - Specifies the number of steps and direction that the motor will
    //           move.  A positive value will move the motor in the clockwise
    //           (CW) direction.  A negative value will move the motor in the
    //           counterclockwise (CCW) direction.
    //   speed - Specifies the speed profile that will be used for the move.
    //             StepSlow selects slow stepper speed.
    //             StepAuto selects automatic stepper speed with accel and decel.
    //             StepFast selects fast stepper speed.
    /////////////////////////////////////////////////////////////////////////////
    void Step(int32_t steps, StepperSpeed_t speed);

    /////////////////////////////////////////////////////////////////////////////
    // IsHome()
    //
    // Returns 'true' if the home sensor is active, based on the type of sensor
    // (N.O. or N.C.) in use.  Returns 'false' otherwise.
    /////////////////////////////////////////////////////////////////////////////
    bool IsHome()          { return ((digitalRead(HOME_PIN) == HIGH) ^ m_InvertHome); }


    /////////////////////////////////////////////////////////////////////////////
    // IsButtonPressed()
    //
    // Returns 'true' if the board's pushbutton is active.  Returns 'false' otherwise.
    /////////////////////////////////////////////////////////////////////////////
    bool IsButtonPressed() { return digitalRead(PUSHBUTTON_PIN) == LOW; }


    // User accessable I/O pin assignments.

    // Note that an instance of RGBLed that uses the pins below is constructed at
    // our instance creation.  In general, the RGBLed instance should be used to
    // control the RGB LEDs.  The following LED pins may be used in special cases,
    // but should be used with caution.
    static const uint8_t LED_RED_PIN   = 13;  // Red LED output pin assignment.
    static const uint8_t LED_GREEN_PIN = 12;  // Green LED output pin assignment.
    static const uint8_t LED_BLUE_PIN  = 27;  // Blue LED output pin assignment.

    // RGBLed instance (public for easy access).
    static RGBLed RgbLed;

    static const uint8_t AUX_1_PIN      = 15;  // Aux1 I/O pin assignment.
    static const uint8_t AUX_2_PIN      = 33;  // Aux2 I/O pin assignment.

    static const int32_t STEP_CW        = 1;   // Clockwise specifier.
    static const int32_t STEP_CCW       = -1;  // Counterclockwise specifier.

protected:


private:
    /////////////////////////////////////////////////////////////////////////////
    // Private methods.
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////
    // Unimplemented methods.  We don't want users to try to use these.
    /////////////////////////////////////////////////////////////////////////////
    GenericClockBoard();
    GenericClockBoard(GenericClockBoard const &);
    GenericClockBoard &operator=(GenericClockBoard &gcm);

    /////////////////////////////////////////////////////////////////////////////
    // Private static constants.
    /////////////////////////////////////////////////////////////////////////////

    // Stepper related constants.
    static const uint8_t PHASE_1_PIN = 19;
    static const uint8_t PHASE_2_PIN = 16;
    static const uint8_t PHASE_3_PIN = 17;
    static const uint8_t PHASE_4_PIN = 21;

    static const uint32_t NUM_STEPPER_PINS = 4;
    static const uint8_t StepperPins[NUM_STEPPER_PINS];
    static const uint8_t StepperPinsReversed[NUM_STEPPER_PINS];

    // I/O pin assignments.
    static const uint8_t HOME_PIN       = 32;   // Home input pin assignment.
    static const uint8_t PUSHBUTTON_PIN = 26;   // Pushbutton input pin assignment.


    /////////////////////////////////////////////////////////////////////////////
    // Private instance data.
    /////////////////////////////////////////////////////////////////////////////
    int32_t  m_CurrentStepperPhase; // Current phase of stepper.
    const uint8_t *m_pStepperPins;  // Stepper pin array.
    uint32_t m_NumStepperPhases;    // Number of stepper phases (4 or 8).
    uint32_t m_StepperRapidDelayUs; // Micros to delay stepper phase update
                                    // for rapid moves.  Slower moves are based
                                    // on multiples of this value.
    uint32_t m_StepperClearMask;    // Bit pattern of stepper pins.
    uint32_t m_StepperSequence[8];  // Sequence of stepper phases to produce
                                    // clockwise motion.
    bool     m_InvertHome;          // True if home switch is N.O.

}; // End class GenericClockBoard

#endif // GENERICCLOCKBOARD_H