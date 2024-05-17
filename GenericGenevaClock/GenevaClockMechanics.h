/////////////////////////////////////////////////////////////////////////////////
// GenevaClockMechanics.h
//
// Declares the GenevaClockMechanics class.  This class directs movement of
// the geneva clock that was originally published by gzumwalt on printables at:
//      https://www.printables.com/model/717033-geneva-clock and
//      https://cults3d.com/en/3d-model/home/geneva-clock
//
// This class derives from the GenericClockBoard class.  Information can be found
// at https://github.com/regnaDkciN/Generic-Clock-Board .
//
// It controls a stepper motor, such as the 28BYJ-48 by updating the clock's
// position once per minute.  It also contains methods to home, and calibrate
// the home position of the clock.
//
// History:
//  - jmcorbett 11-MAY-2024
//    Original creation.
//
// Copyright (c) 2024, Joseph M. Corbett
//
/////////////////////////////////////////////////////////////////////////////////
#if !defined GENEVACLOCKMECHANICS_H
#define GENEVACLOCKMECHANICS_H

#include <time.h>               // For tm structure.
#include <Arduino.h>            // For digitalRead() ...
#include "GenericClockBoard.h"  // For GenericClockBoard class.


/////////////////////////////////////////////////////////////////////////////////
// StatusCode_t
//
// This enum is used to specify status/error codes as follows:
//  0 - Success.
//  1 - Homing phase 1 error.  Could not find home sensor after moving CW for
//                             more than 13 hours.
//  2 - Homing phase 2 error.  Could not move off home sensor in the CCW direction.
//  3 - Homing phase 3 error.  Could not re-find home sensor after moving off.
/////////////////////////////////////////////////////////////////////////////////
enum StatusCode_t
{
    StatusSuccess = 0,      // Successful operation.
    StatusHomePhase1Error,
    StatusHomePhase2Error,
    StatusHomePhase3Error,
};


/////////////////////////////////////////////////////////////////////////////////
// GenevaClockMechanics class
//
// This class derives from the GenericClockBoard class, and provides methods
// related to the homing and updating of the Geneva Clock.
/////////////////////////////////////////////////////////////////////////////////
class GenevaClockMechanics : public GenericClockBoard
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
    GenevaClockMechanics(
        uint32_t rapidSecondsPerRev,    // Number of seconds for fastest motor rev.
        uint32_t fullStepsPerRev,       // Number of full steps per motor revolution.
        bool     stepperPinsReversed,   // True if servo runs backwards.
        bool     stepperHalfStepping,   // True for half stepping, false for full.
        bool     homeNormallyOpen);     // True if home switch is normally open.


    // Destructor.
    ~GenevaClockMechanics() {}


    /////////////////////////////////////////////////////////////////////////////
    // UpdateClock()
    //
    // Updates the position of the clock indicator based on the current time.
    // Assuming that the clock has been homed at some point in the past, when
    // passed the current time, this method will do the following:
    //  - Determine the difference, in minutes, between the current time and the
    //    last time the method was called.
    //  - Move the time indicator the correct number of steps, in the shortest
    //    distance possible, to the new time.
    //
    // Arguments:
    //  - localTime is the current time.
    /////////////////////////////////////////////////////////////////////////////
    void UpdateClock(tm &localTime);


    /////////////////////////////////////////////////////////////////////////////
    // Home()
    //
    // Home the clock to the 12:00 position.  We want to always approach the home
    // switch slowly in the clockwise direction to achieve the best repeatability.
    //  The strategy here is as follows:
    //   - If we are not already on the home, then move rapidly clockwise toward
    //     the home till the home switch is detected.
    //   - Rapidly back off the home switch in the counterclockwise direction
    //     until the home switch is no longer detected.
    //   - Slowly approach the home in the clockwise direction until the home
    //     switch is detected.
    //
    // Returns:
    // Returns a status code as follows:
    //  0 - Success.
    //  1 - Homing phase 1 error.  Could not find home sensor after moving CW for
    //      more than 13 hours.
    //  2 - Homing phase 2 error.  Could not move off home sensor in the CCW
    //      direction.
    //  3 - Homing phase 3 error.  Could not re-find home sensor after moving off.
    /////////////////////////////////////////////////////////////////////////////
    StatusCode_t Home();


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
    void Calibrate();

protected:


private:
    /////////////////////////////////////////////////////////////////////////////
    // Private methods.
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////
    // Unimplemented methods.  We don't want users to try to use these.
    /////////////////////////////////////////////////////////////////////////////
    GenevaClockMechanics();
    GenevaClockMechanics(GenevaClockMechanics const &);
    GenevaClockMechanics &operator=(GenevaClockMechanics &gcm);

    /////////////////////////////////////////////////////////////////////////////
    // Private static constants.
    /////////////////////////////////////////////////////////////////////////////
    static const uint32_t MINUTES_PER_HOUR  = 60;   // 60 minutes per hour.
    static const uint32_t HOURS_PER_REV     = 3;    // 3 hours per moror rev.
    static const uint32_t HOURS_PER_CYCLE   = 12;   // 12 hours per cycle.
    static const  int32_t MINUTES_PER_CYCLE = MINUTES_PER_HOUR * HOURS_PER_CYCLE;
                                                    // Number minutes per cycle.
    static const uint32_t GEAR_RATIO        = 32 / 8;  // Main gear 32, motor 8.


    /////////////////////////////////////////////////////////////////////////////
    // Private instance data.
    /////////////////////////////////////////////////////////////////////////////
    int32_t  m_LastStepperPos;      // Last updated position of motor, in steps.
    int32_t  m_LastMinutes;         // Last updated time, in minutes
                                    // Should normally be -719 through 719.
    uint32_t m_StepsPerHour;        // Number of motor steps per hour.
    int32_t  m_StepsPerCycle;       // Number of motor steps per 12 hours.


}; // End class GenevaClockMechanics.


#endif // GENEVACLOCKMECHANICS_H