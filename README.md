# Geneva Clock

An ESP32 based Geneva Clock controller.  The motivation for this project was gzumwalt's excellent [Geneva Clock which was originally published on printables](https://www.printables.com/model/717033-geneva-clock), and [on cults3d](https://cults3d.com/en/3d-model/home/geneva-clock).

This version of the Geneva Clock has the following features:
- Uses the [GenericClockBoard](https://github.com/regnaDkciN/Generic-Clock-Board) hardware and support library.
- Fetches accurate NTP time from the internet, if available, to keep accurate time.
- Uses the optioinal DS3231 Real Time Clock (RTC) module for backup when no WiFi is available.
- Uses the [Arduino WiFiTimeManager library](https://github.com/regnaDkciN/WiFiTimeManager) to handle DST changes automatically.
-  Automatically homes to 12:00 upon power-up, then seeks to the current time.
- Uses the RGB LED for status display.
- Uses the pushbutton input for general special operation.
- Optionally homes to 12:00 twice a day to keep in sync with the correct time.
- Uses the GenevaClockMechanics library written in C++ to control the clock motor.
- Includes a control box to house the Generic Clock Board.  The clock's base was also modified in order to mate with the new control box.  New .stl files for these parts are included.

---
## Introduction
This clock displays the current time in a novel way as described by [gzumwalt](https://www.printables.com/model/717033-geneva-clock).  Where it differs is mainly in the controller wiring, and the presence of an RGB LED status display and pushbutton.
### Controller Board
This clock uses a [controller board](https://github.com/regnaDkciN/Generic-Clock-Board) that encapsulates the ESP32, motor controller, RTC, LED, and pushbutton.  The addition of the RTC, LED, and pushbutton  (hopefully) adds capability and simplifies the interface to the clock.
- The RTC adds the ability to start with the correct time, even when no WiFi is available.
- The RGB LED is used to give an indication of what is happening within the clock as a rudimentary debug tool.  The following indications may be generated:
	+ At power-up, the LEDs will each fade in then out once in a Blue, Green, Red sequence in order to show that the LEDs are functioning properly.
	+ Solid Blue indicates that the clock is working and using time from an NTP time server.
	+ Solid Green indicates that the clock is working and using time from the internal RTC (i.e. WiFi is not connected or cannot find the NTP server).
	+ Single flash Magenta indicates that a time update was retrieved from the NTP server.
	+ Solid White indicates that a home or calibration operation is in process.
	+ Repeated flashing Red indicates that a fatal error has occurred.  The number of flashes indicates the problem:
		* 1 flash indicates that the home sensor could not be found while trying to home the clock to 12:00.
		* 2 flashes indicate that the home sensor did not deactivate after finding the initial home sensor activated while homing.
		* 3 flashes indicate that the home sensor could not be found when re-approaching the sensor while homing.
		* 5 flashes indicate that the RTC could not be read.
- The pushbutton has several uses based on the situation as follows:
	+ If the pushbotton is pressed immediately upon power-up, a calibration procedure is initiated.  The calibration helps in determining the proper placement of the home sensor.  It repeatedly homes the clock, then delays for several seconds to allow for inspection and readjustment of the home sensor position.  After the delay, it moves the clock backwards by one hour and repeats the process.  The calibration procedure may be exited by pressing the pushbutton again.
	+ While an error is being displayed, a press of the pushbutton will cause the board to restart.  This will hopefully clear the error.
	+ During normal operation, a short press of the pushbutton (less than 3 seconds) will restart the configuration portal and will cause the clock to home.
	+ During normal operation, a long press of the pushbutton (greater than 3 seconds) will cause all saved DST and WiFi related data to be erased, and will restart the board.  This will cause the config portal to be active on the resulting startup, and will require re-entry of WiFi and DST related information.  BE CAREFUL USING THIS SINCE IT WILL DELETE ALL PREVIOUSLY SAVED CONFIGURATION DATA.



---
## Geneva Clock Mechanics Class

The GenevaClockMechanics class is mainly used to initialize and control the Geneva Clock's 28BYJ-48 stepper motor such that it indicates the correct time.  It publicly derives from the [GenericClockBoard library](https://github.com/regnaDkciN/Generic-Clock-Board), so all of the GenericClockBoard public data and methods are available.

The Geneva Clock Mechanics class consists of 2 files - GenevaClockMechanics.h and GenevaClockMechanics.cpp.  It requires the following libraries to be installed:
- [RGBLed Library](https://github.com/wilmouths/RGBLed): This library supports the board's RGB LEDs and provides dimming and color mixing support.
- [SerialDebug Library](https://github.com/JoaoLopesF/SerialDebug ): This library supports serial debugging and status display.

Since the GenevaClockMechanics class publicly derives form the GenericClockBoard class, all of the GenericClockBoard public data and methods are available.  This section describes the overridden methods and data,

### Constructor
Constructs a class instance, initializes board hardware, and initializes instance variables.
#### Constructor Arguments:
 - *__rapidSecondsPerRev__*  - (uint32_t) Specifies the number of seconds it takes the stepper to make one full revolution of its output shaft.  For the 28BYJ-48 stepper motor, a good range is normally between 6 and 10 seconds.
- *__fullStepsPerRev__* - (uint32_t) Specifies the number of FULL steps per revolution of the stepper motor's output shaft.  For the   28BYJ-48 the value is 2048.
- *__stepperPinsReversed__* - (bool) Specifies the whether or not the stepper turns clockwise when a positive step value is commanded.  Set to 'true' if a positive step value causes counterclockwise movement.  Set to 'false' otherwise.
- *__stepperHalfStepping__* - (bool) Specifies whether half stepping is to be used.  If 'true', then half stepping is used, which will cause the number of steps per rev of the stepper to double.  For example, the 28BYJ-48 stepper will take 4096 steps per rev if this value is set to 'true'.  In most cases, use of half stepping is a good choice.
- *__homeNormallyOpen__* - (bool) Specifies the type of sensor used for homing the clock.  Set to 'true' for normally open  (N.O.) sensors.  Set to 'false' for normally closed (N.C.) sensors.

#### Constructor Example
```
// RAPID_SECONDS_PER_REV specifies the number of seconds it takes for the stepper
// motor to complete 1 full revolution at its maximum speed.  A good value for
// the 28BYJ-48 stepper motor is usually in the range of 6 to 10 seconds.
const uint32_t RAPID_SECONDS_PER_REV = 8;

// 28BYJ-48 has 2048 full steps per full rev of the output shaft (4096 half steps).
const uint32_t FULL_STEPS_PER_REV = 2048;

// This stepper needs to have the phases reversed.  Set to 'false' if stepper runs
// backwards.
const bool REVERSE_STEPPER = true;

// USE_HALF_STEPPING selects the use of half stepping of the stepper motor.
// Half stepping is recommended, and is specified by setting this constant to
// ture.  If full stepping is desired, set it to false.
const bool USE_HALF_STEPPING = true;

// The home sensor is normally open.  Set to false if normally closed.
const bool HOME_SWITCH_NORMALLY_OPEN = true;

GenevaClockMechanics gClock(RAPID_SECONDS_PER_REV, FULL_STEPS_PER_REV,  
                            REVERSE_STEPPER, USE_HALF_STEPPING, HOME_SWITCH_NORMALLY_OPEN);
```

### UpdateClock()
Updates the position of the clock indicator based on the current time.   Assuming that the clock has been homed at some point in the past, when passed the current time, this method will do the following:
- Determine the difference, in minutes, between the current time and the last time the method was called.
- Move the time indicator the correct number of steps, in the shortest distance possible, to the new time.

#### UpdateClock() Arguments:
- *__localTime__*  (tm &) is the current time.

#### UpdateClock() Example
```
    // Update the time and run the clock's mechanics.
    tm now;
    gpWtm->GetLocalTime(&now);
    gClock.UpdateClock(now);
```

### Home()
Homes the clock to the 12:00 position.  We want to always approach the home switch slowly in the clockwise direction to achieve the best repeatability.  The strategy here is as follows:
- If we are not already on the home, then move rapidly clockwise toward the home till the home switch is detected.
- Rapidly back off the home switch in the counterclockwise direction until the home switch is no longer detected.
- Slowly approach the home in the clockwise direction until the home switch is detected.

#### Home() Returns
Returns a status code (StatusCode_t) as follows:
- 0 - Success.
- 1 - Homing phase 1 error.  Could not find home sensor after moving CW for more than 13 hours.
- 2 - Homing phase 2 error.  Could not move off home sensor in the CCW direction.
- 3 - Homing phase 3 error.  Could not re-find home sensor after moving off.

#### Home() Example
```
StatusCode_t status = gClock.Home();
```

### Calibrate()
This method is used to assist in calibrating the home sensor position.  It repeatedly homes the clock, then delays for several seconds to allow for inspection and readjustment of the home sensor position.  After the delay, it moves the clock backwards by one hour and repeats the process.

The GenericClockBoard pushbutton may be pressed in order to exit the otherwise infinite calibration loop.

#### Calibrate() Example
```
    // If the pushbutton is pressed, then perform a home calibration.
    // The red LED will light when the calibration request is detected.  Release
    // the pushbutton before the red LED goes out (2 seconds) in order for the
    // calibration to begin.
    if (gClock.IsButtonPressed())
    {
        gClock.RgbLed.brightness(RGBLed::WHITE, 2);
        delay(2000);
        gClock.Calibrate();
    }
```

### StatusCode_t enum
This enum is used to specify status/error codes as follows:
- 0 - Success.
- 1 - Homing phase 1 error.  Could not find home sensor after moving CW for more than 13 hours.
- 2 - Homing phase 2 error.  Could not move off home sensor in the CCW direction.
- 3 - Homing phase 3 error.  Could not re-find home sensor after moving off.

---

## Generic Geneva Clock Example
Example code to run the Geneva clock is attached in the file GenericGenevaClock.ino.  It is fairly well commented, and has run for several months on a local system.  It implements the full clock capability set including:
- Uses the RTC.
- Uses the WiFiTimeManager library to manage WiFi network credentials, NTP access, and automatic DST adjustment.
- Uses the RGB LED to display status as described above.
- Uses the Pushbutton as described above.

An option is to have the clock re-home twice a day at 12:00.  This option is enabled by default.  To disable this option, simply comment out the following line in *__"GenericGenevaClock.ino"__*:
```
// Comment out the following line if homing the clock each 12:00 is not wanted.
#define HOME_AT_12 1
```

---
## 3D Print Parts
A new control box was added to gzimwalt's original design to hold the Generic Clock Board.  OpenSCAD files as well as .stl files are included.  This section details the new and modified parts.

### Geneva Clock Box.scad
This file was used to create the control box and its lid.  The box and lid must be created separately.  The very bottom of the file contains the selection for box or lid:
```
!finalBox();     // Here we generate the box
roundedLid();    // and not the lid.
```
Insert an exclamation point before the line that specifies either the box (finalBox) or lid (roundedLid) in order to generate the desired part.  The .stl files for the box and lid are included as *__"Geneva Clock Box.stl"__* and *__"Geneva Clock Box Lid.stl"__*.

Note that the lid assumes that the Huzzah32 board is socketed.  Boards that directly solder the Huzzah32 to the Generic Clock Board will need to adjust the location of the USB opening.


### StandModified.scad
The original design original stand needed to be modified.  The mounting plate for the original board was removed, mounting posts were added, the back of the base was extended, and the hole that accepted the back was filled in at the bottom.  The latter modification was found to be useful to keep the back from sliding too far through the base hole.  The *__"StandModified.scad"__* file is used to affect these changes.  The *__"StandModified.stl"__* file contains the modiried base.  It should be used in place of the original *__"Stand.stl"__* file.


### Face Mount.scad
The mounts for the face presented a problem for my printer.  I don't have a multi-color printer, so I generated separate face mounts.  Unfortunately, when I inserted them into the face, the mounting nib broke off.  I decided that a better solution would be to attach the face mounts directly to the face, and fill the '15' and '55' holes with black plugs.   I have not tried this solution, but if I make another one I will use it.  Anyway, the *__"Face Mount.scad"__* file can be used to generate the face with the mounts attached.  The attached *__"Face With Mounts.stl"__* file contains such a part.  When printing, just change the color to black when the face height is reached.  *__"Face Mount.scad"__* can also be used to generate an individual mounting post with the mounting nib if desired.  The *__"Full Post.stl"__* file is such a part.


### LEDHolder.scad
While not strictly necessary, the *__"LEDHolder.scad"__* file will generate a holder for the RGB LED.  This holder is used to make sure that the mounting height of the LED matches the corresponding hole in the control box lid.  The resulting part is *__"LEDMount.stl"__*.

