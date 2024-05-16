/////////////////////////////////////////////////////////////////////////////////
// GenericClockBoard.cpp
//
// Contains the implementation of the GenericClockBoard class.  This class
// defines the base interface to the Generic Clock Board.  See %%%jmc
// It is meant  to control any type of motorized clock, and supports automatic
// DST handling via the attached ESP32 processor and the WiFiTimeManager library
// at https://github.com/regnaDkciN/WiFiTimeManager.
//
// It is meant to control any type of 5 volt stepper, but has only been tested
// with, a 28BYJ-48 stepper motor.  Other stepper motors may work, but some
// constants may need to be changed.
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
//    Original code.
//
// Copyright (c) 2024, Joseph M. Corbett
//
/////////////////////////////////////////////////////////////////////////////////

#include "GenericClockBoard.h"      // For GenericClockBoard class.

// GenericClockBoard static definitions.
const uint8_t GenericClockBoard::StepperPins[NUM_STEPPER_PINS] =
            {PHASE_1_PIN, PHASE_2_PIN, PHASE_3_PIN, PHASE_4_PIN};
const uint8_t GenericClockBoard::StepperPinsReversed[NUM_STEPPER_PINS] =
            {PHASE_4_PIN, PHASE_3_PIN, PHASE_2_PIN, PHASE_1_PIN};
RGBLed GenericClockBoard::RgbLed(
    LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN, RGBLed::COMMON_CATHODE);



/////////////////////////////////////////////////////////////////////////////////
// GenericClockBoard()  (constructor)
//
// Constructs a class instance, initializes board hardware, and initializes
// instance variables.
//
// Arguments:
//   - rapidSecondsPerRev  - Specifies the number of seconds it takes the stepper
//                           to make one full revolution of its outupt shaft.
//                           For the 28BYJ-48 stepper motor, a good range is
//                           normally between 6 and 10 seconds.
//   - fullStepsPerRev     - Specifies the number of FULL steps per revolution of
//                           stepper motor's output shaft.  For the 28BYJ-48
//                           the value is 2048.
//   - stepperPinsReversed - Specifies the whether or not the stepper turns
//                           clockwise when a positive step value is commanded.
//                           Set to 'true' if a positive step value causes
//                           counterclockwise movement.  Set to 'false' otherwise.
//   - stepperHalfStepping - Specifies whether half stepping is to be used.  It
//                           'true', then half stepping is used, which will cause
//                           the number of steps per rev of the stepper to double.
//                           For example, the 28BYJ-48 stepper will take 4096
//                           steps per rev if this value is set to 'true'.  In
//                           most cases, use of half stepping is a good choice.
//   - homeNormallyOpen    - Specifies the type of sensor used for homing the
//                           clock.  Set to 'true' for normally open (N.O.)
//                           sensors.  Set to 'false' for normally closed (N.C.)
//                           sensors.
/////////////////////////////////////////////////////////////////////////////////
GenericClockBoard::GenericClockBoard(
    uint32_t rapidSecondsPerRev,    // Number of seconds for fastest motor rev.
    uint32_t fullStepsPerRev,       // Number of full steps per motor revolution.
    bool     stepperPinsReversed,   // True if servo runs backwards.
    bool     stepperHalfStepping,   // True for half stepping, false for full.
    bool     homeNormallyOpen) :    // True if home switch is normally open.
             m_CurrentStepperPhase(0)
{
    // Save a pointer to the proper motor pins array and initialize them as OUTPUTs.
    m_pStepperPins = stepperPinsReversed ? StepperPinsReversed : StepperPins;
    for (uint32_t i = 0; i < NUM_STEPPER_PINS; i++)
    {
        pinMode(m_pStepperPins[i], OUTPUT);
        digitalWrite(m_pStepperPins[i], LOW);
    }

    // Half stepping uses 8 phases, full stepping uses 4.
    m_NumStepperPhases = stepperHalfStepping ? 8 : 4;

    // Initialize motor step related class data.
    uint32_t stepsPerRev = fullStepsPerRev * (stepperHalfStepping ? 2 : 1);

    const uint32_t US_PER_SEC = 1000000;
    m_StepperRapidDelayUs =  US_PER_SEC * rapidSecondsPerRev / stepsPerRev;

    // Macro to create a bit pattern from a port number.
    #define PIN_BP(p) (1UL << m_pStepperPins[p])
    m_StepperClearMask = PIN_BP(0) | PIN_BP(1) | PIN_BP(2) | PIN_BP(3);

    // Initialize the motor stepping phases.
    for (uint32_t i = 0; i < NUM_STEPPER_PINS; i++)
    {
        if (m_NumStepperPhases == 4)
        {
            m_StepperSequence[i] = PIN_BP(i);
        }
        else
        {
            uint32_t j = 2 * i;
            m_StepperSequence[j]     = PIN_BP(i);
            m_StepperSequence[j + 1] = PIN_BP(i) | PIN_BP((i + 1) % NUM_STEPPER_PINS);
        }
    }

    // Initialize the home and pushbutton inputs.
    m_InvertHome = homeNormallyOpen;
    pinMode(HOME_PIN, INPUT_PULLUP);
    pinMode(PUSHBUTTON_PIN, INPUT_PULLUP);

} // End GenericClockBoard()


/////////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////////
void GenericClockBoard::Step(int32_t steps, StepperSpeed_t speed)
{
    if (!steps)
    {
        GPIO.out_w1tc = m_StepperClearMask;
        return;
    }

    // Use modulo arithmatic to make the stepper move in the selected direction.
    int32_t delta = (steps > 0) ? 1 : (m_NumStepperPhases - 1);

    // Since 'delta' is used to affect the motor direction, we only need to
    // use the magnitude of the move for the rest of the function.
    int32_t absSteps = abs(steps);

    // Output the specified number of steps applying accel and decel as needed.
    for (int32_t j = 0; j < absSteps; j++)
    {
        // Increment the stepper phase and wrap as needed.
        m_CurrentStepperPhase = (m_CurrentStepperPhase + delta) % m_NumStepperPhases;

        // Output the new phase to the stepper.
        GPIO.out_w1ts = m_StepperSequence[m_CurrentStepperPhase];

        // Assume we are making a fast move and make an initial delay.
        delayMicroseconds(m_StepperRapidDelayUs);

        // For slow speed, add an additional delay.
        if (speed == StepSlow)
        {
            delayMicroseconds(m_StepperRapidDelayUs * 4);
        }
        else if (speed == StepAuto)
        {
            // Delay based on accel/decel.  Seems like all phases should be
            // disabled after the first delay to reduce heat and power use of
            // the stepper.  However, this led to missed steps, so it was
            // moved below.
            if (j < 20)            delayMicroseconds(m_StepperRapidDelayUs); // Acceleration
            if (j < 10)            delayMicroseconds(m_StepperRapidDelayUs); // Acceleration
            if (j < 5)             delayMicroseconds(m_StepperRapidDelayUs); // Acceleration
            if (absSteps - j < 20) delayMicroseconds(m_StepperRapidDelayUs); // Deceleration
            if (absSteps - j < 10) delayMicroseconds(m_StepperRapidDelayUs); // Deceleration
            if (absSteps - j < 5)  delayMicroseconds(m_StepperRapidDelayUs); // Deceleration
        }

        // Disable all stepper phases.
        GPIO.out_w1tc = m_StepperClearMask;
    }

} // End Step().


