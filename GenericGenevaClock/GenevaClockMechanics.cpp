/////////////////////////////////////////////////////////////////////////////////
// GenevaClockMechanics.cpp
//
// Contains the implementation of the GenevaClockMechanics class.  This class
// is used to control a Geneva Clock.  See
//     (https://www.printables.com/model/717033-geneva-clock and
//      https://cults3d.com/en/3d-model/home/geneva-clock)
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
// History:
//  - jmcorbett 11-MAY-2024
//    Original code.
//
// Copyright (c) 2024, Joseph M. Corbett
//
/////////////////////////////////////////////////////////////////////////////////

#include "GenevaClockMechanics.h"   // For GenevaClockMechanics class.


/////////////////////////////////////////////////////////////////////////////////
// GenevaClockMechanics()  (constructor)
//
// Arguments:
//   - motorPins[NUM_MOTOR_PINS]
//   - homePin
//   - invertHome
//   - rapidSecondsPerRev
//   - numMotorPhases
/////////////////////////////////////////////////////////////////////////////////
GenevaClockMechanics::GenevaClockMechanics(
    uint32_t rapidSecondsPerRev,    // Number of seconds for fastest motor rev.
    uint32_t fullStepsPerRev,       // Number of full steps per motor revolution.
    bool     stepperPinsReversed,   // True if servo runs backwards.
    bool     stepperHalfStepping,   // True for half stepping, false for full.
    bool     homeNormallyOpen) :    // True if home switch is normally open.
             GenericClockBoard(rapidSecondsPerRev, fullStepsPerRev,
                               stepperPinsReversed, stepperHalfStepping,
                               homeNormallyOpen),
             m_LastStepperPos(0), m_LastMinutes(0)
{
    // Initialize motor step related class data.
    uint32_t stepsPerRev = fullStepsPerRev * (stepperHalfStepping ? 2 : 1);
    m_StepsPerHour       = (stepsPerRev * GEAR_RATIO) / HOURS_PER_REV;

    // HOURS_PER_REV has a value of 3 in this implementation.  We use the following
    // calculation where the grouping of the division is important in order to
    // cancel out the factor of 3.  This eliminates a remainder in the division
    // and insures that m_StepsPerCycle is an exact integer.
    //
    // NOTE THAT THIS IS HIGHLY IMPLEMENTATION SPECIFIC, AND MAY NOT WORK IN
    // IMPLEMENTATIONS WHERE OTHER RATIOS ARE USED.
    m_StepsPerCycle = stepsPerRev * GEAR_RATIO * (HOURS_PER_CYCLE / HOURS_PER_REV);

} // End GenevaClockMechanics()


/////////////////////////////////////////////////////////////////////////////////
// UpdateClock()
//
// Updates the position of the clock indicator based on the current time.
// Assuming that the clock has been homed at some point in the past, when passed
// the current time, this method will do the following:
//  - Determine the difference, in minutes, between the current time and the
//    last time the method was called.
//  - Move the time indicator the correct number of steps, in the shortest
//    distance possible, to the new time.
//
// Arguments:
//  - localTime is the current time.
/////////////////////////////////////////////////////////////////////////////////
void GenevaClockMechanics::UpdateClock(tm &localTime)
{
    // Calculate the number of minutes since 12:00.
    int32_t newTimeInMinutes = (
        (localTime.tm_hour % HOURS_PER_CYCLE) * MINUTES_PER_HOUR) + localTime.tm_min;

    // Check if update is needed (i.e. has time changed?).
    if(newTimeInMinutes != m_LastMinutes)
    {
        // Remember the current time for next iteration.
        debugD("newTimeInMinutes = %d,   %02d:%02d",
            newTimeInMinutes, localTime.tm_hour, localTime.tm_min);
        m_LastMinutes = newTimeInMinutes;

        // Determine the number of steps corresponding to the new time in minutes.
        int32_t newMotorPos =
            ((newTimeInMinutes * m_StepsPerCycle) / MINUTES_PER_CYCLE);
        debugD("newMotorPos = %d", newMotorPos);

        // Calculate the number of steps and direction between the new time and
        // the old time.
        int32_t deltaSteps = newMotorPos - m_LastStepperPos;
        debugD("deltaSteps = %d, m_LastStepperPos = %d", deltaSteps, m_LastStepperPos);

        if (deltaSteps > m_StepsPerCycle / 2)
        {
            deltaSteps -= m_StepsPerCycle;
        }
        else if (deltaSteps < -m_StepsPerCycle / 2)
        {
            deltaSteps += m_StepsPerCycle;
        }

        // Actually move the time indicator the number of steps required to get
        // to the new time.
        debugD("Step(%d, StepAuto);", deltaSteps);
        Step(deltaSteps, StepAuto);

        // Remember the last step position for next iteration.
        m_LastStepperPos = (m_LastStepperPos + deltaSteps) % m_StepsPerCycle;
        debugD("m_LastStepperPos = %d\n", m_LastStepperPos);
    }
} // End UpdateClock().


/////////////////////////////////////////////////////////////////////////////////
// Home()
//
// Home the clock to the 12:00 position.  We want to always approach the home
// switch slowly in the clockwise direction to achieve the best repeatability.
//  The strategy here is as follows:
//   - If we are not already on the home, then move rapidly clockwise toward the
//     home till the home switch is detected.
//   - Rapidly back off the home switch in the counterclockwise direction until
//     the home switch is no longer detected.
//   - Slowly approach the home in the clockwise direction until the home switch
//     is detected.
//
// Returns:
// Returns a status code as follows:
//  0 - Success.
//  1 - Homing phase 1 error.  Could not find home sensor after moving CW for
//      more than 13 hours.
//  2 - Homing phase 2 error.  Could not move off home sensor in the CCW direction.
//  3 - Homing phase 3 error.  Could not re-find home sensor after moving off.
/////////////////////////////////////////////////////////////////////////////////
StatusCode_t GenevaClockMechanics::Home()
{
    // Debug.
    printlnV("HomeClock(): homing clock to 12:00.");

    // Phase 1, move rapidly CW till home is detected.  Return with an error if
    // home is not detected within a reasonable distance.
    uint32_t i = 0;
    const uint32_t MAX_STEPS = m_StepsPerCycle + m_StepsPerHour;
    for (i = 0; !IsHome() && (i < MAX_STEPS); i++)
    {
        Step(STEP_CW, StepFast);
    }
    if (i >= MAX_STEPS)
    {
        printlnE("Home phase 1 error.");
        return StatusHomePhase1Error;
    }

    // Phase 2, move rapidly off the home switch in the CCW direction.  Return
    // with an error if home is not removed within a reasonable distance.
    for (i = 0; IsHome() && (i < m_StepsPerHour); i++)
    {
        Step(STEP_CCW, StepFast);
    }
    if (i >= m_StepsPerHour)
    {
        printlnE("Home phase 2 error.");
        return StatusHomePhase2Error;
    }

    // Phase 3, move slowly back to home in the CW direction.  Return with an
    // error if home is not detected within a reasonable distance.
    for (i = 0; !IsHome() && (i < m_StepsPerHour); i++)
    {
        Step(STEP_CW, StepSlow);
    }
    if (i >= m_StepsPerHour)
    {
        printlnE("Home phase 3 error.");
        return StatusHomePhase3Error;
    }

    // Homed successfully.  Reset the current time and stepper position to zero.
    m_LastStepperPos = 0;
    m_LastMinutes  = 0;

    printlnV("Done homing.");

    return StatusSuccess;
} // End Home().


/////////////////////////////////////////////////////////////////////////////
// Calibrate()
//
// This method is used to assist in calibrating the home sensor position.
// It repeatedly homes the clock, then delays for several seconds to allow
// for inspection and readjustment of the home sensor position.  After the
// delay, it moves the clock backwards by one hour and repeats the process.
//
// The GenericClockBoard pushbutton may be pressed in order to exit the
// otherwise infinite calibration loop.
/////////////////////////////////////////////////////////////////////////////
void GenevaClockMechanics::Calibrate()
{
    printlnV("Calibrating.");
    while (!IsButtonPressed())
    {
        Home();
        if (IsButtonPressed()) break;
        delay(10000);
        if (IsButtonPressed()) break;
        Step(-m_StepsPerHour, StepFast);
        if (IsButtonPressed()) break;
        delay(500);
    }
    printlnV("Done calibrating.");
} // End Calibrate().

