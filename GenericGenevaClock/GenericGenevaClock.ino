/////////////////////////////////////////////////////////////////////////////////
// GenericGenevaClock.ino
//
// This file contains an adaptation of the code to support the Geneva Clock
// that was originally published by gzumwalt on printables at:
//      https://www.printables.com/model/717033-geneva-clock and
//      https://cults3d.com/en/3d-model/home/geneva-clock
// This version makes the following modifications:
//      1. Uses the GenericClockBoard hardware at
//         https://github.com/regnaDkciN/Generic-Clock-Board .
//      2. Uses the WiFiTimeManager library to handle timezone, DST, and NTP
//         maintenance: https://github.com/regnaDkciN/WiFiTimeManager .
//      3. Adds an optional real time clock (RTC) as a backup in case network
//         connections fails.
//      4. Broke out motor interface code into a separate class
//         (GenevaClockMechanics) that uses different algorithms for motor
//         stepping and clock positioning.
//      5. 28BYJ-48 stepper motors are geared such that the actual steps
//         per revolution using half steps are 4075.52 as opposed to the 4096
//         that is normally used in Arduino code.  Due to this, there is some
//         daily clock drift.  To accommodate this, the clock can be optionally
//         commanded to perform a homing operation periodically at 12:00.
//
// Note that this implementation relies heavily on the use of an ESP32.  Most
// ESP32 boards should work with the code.  However, the code probably won't
// work with other processors, including the ESP8266.
//
// History:
//  - jmcorbett 11-MAY-2024
//    - Use RGBLed library for RGB LED outputs to reduce their intensity.
//    - Made use of the GenericClockBoard library.
//    - Added clock calibration option which will run if the pushbutton is pressed
//      at startup.
//    - Reworked/completed pushbutton operation.
//    - Other minor code cleanup.
//  - jmcorbett 16-FEB-2024
//    Original code.
//
// Copyright (c) 2024, Joseph M. Corbett
//
/////////////////////////////////////////////////////////////////////////////////

#include <String>                   // For String class.
#include <WiFiTimeManager.h>        // Manages timezone, DST, and NTP.
#include "GenevaClockMechanics.h"   // For GenevaClockMechanics (clock mechanics).


/////////////////////////////////////////////////////////////////////////////////
// USER SETTABLE CONSTANTS  (CHANGE THESE DEPENDING ON YOUR HARDWARE SETUP)
/////////////////////////////////////////////////////////////////////////////////

// *** Comment out the following line if no RTC is connected. ***
#define USE_RTC 1

// RAPID_SECONDS_PER_REV specifies the number of seconds it takes for the stepper
// motor to complete 1 full revolution at its maximum speed.  A good value for
// the 28BYJ-48 stepper motor is usually in the range of 6 to 10 seconds.
static const uint32_t RAPID_SECONDS_PER_REV = 8;

// 28byj-48 has 2048 full steps per full rev of the output shaft (4096 half steps).
static const uint32_t FULL_STEPS_PER_REV = 2048;

// This stepper needs to have the phases reversed.  Set to 'false' if stepper runs
// backwards.
static const bool REVERSE_STEPPER = true;

// USE_HALF_STEPPING selects the use of half stepping of the stepper motor.
// Half stepping is recommended, and is specified by setting this constant to
// ture.  If full stepping is desired, set it to false.
static const bool USE_HALF_STEPPING = true;

// The home sensor is normally open.  Set to false if normally closed.
static const bool HOME_SWITCH_NORMALLY_OPEN = true;

// Comment out the following line if homing the clock each 12:00 is not wanted.
#define HOME_AT_12 1

// Define aliases for RGB color arrays for better code readability.
#define NTP_CLOCK_LED   RGBLed::BLUE   // NTP clock LED color = blue.
#define LOCAL_CLOCK_LED RGBLed::GREEN  // Local clock LED color = green.
#define ERROR_LED       RGBLed::RED    // Error LED color = red.

/////////////////////////////////////////////////////////////////////////////////
// END OF USER SETTABLE CONSTANTS
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
// Local global variables.
/////////////////////////////////////////////////////////////////////////////////

// Construct the GenevaClockMechanics instance that controls the clock's motor.
static GenevaClockMechanics
   gClock(RAPID_SECONDS_PER_REV, FULL_STEPS_PER_REV, REVERSE_STEPPER,
        USE_HALF_STEPPING, HOME_SWITCH_NORMALLY_OPEN);


/////////////////////////////////////////////////////////////////////////////////
// WiFiTimeManager related constants and variables.
/////////////////////////////////////////////////////////////////////////////////

static WiFiTimeManager *gpWtm;  // Pointer to the WiFiTimeManager singleton instance.
static const bool SETUP_BUTTON  = true;
                                // Use a separate Setup button on the web page.
static const bool BLOCKING_MODE = false; // Use non-blocking mode.
static const char *AP_NAME = "Geneva Clock Setup";
                                // AP name user will see as network device.
static const char *AP_PWD  = NULL;
                                // AP password.  NULL == no password.



/////////////////////////////////////////////////////////////////////////////////
// Special Real Time Clock (RTC) code.
/////////////////////////////////////////////////////////////////////////////////
#if defined USE_RTC

    #include <DS323x_Generic.h> // https://github.com/khoih-prog/DS323x_Generic
    static DS323x gRtc;         // The DS3231 RTC instance.


    /////////////////////////////////////////////////////////////////////////////
    // SetupRtc()
    //
    // Initialize the RTC, perform a sanity check, and setup WiFiTimeManager
    // callbacks in order to get the RTC ready to use.
    //
    // Arguments:
    //   rRtc - This is a reference to the RTC instance to be initialized.
    //   pWtm - This is a pointer to the WiFiTimeManager instance used in this
    //          system.
    //
    // Returns:
    //   Returns a value of 0 on successful completion, or a value of RTC_ERROR
    //   (5) on failure.
    /////////////////////////////////////////////////////////////////////////////
    uint32_t SetupRtc(DS323x &rRtc, WiFiTimeManager *pWtm)
    {
        // Initialize I2C for the RTC.
        Wire.begin();
        rRtc.attach(Wire);

        // Simple sanity check - see if RTC interface is functioning OK by reading
        // its temperature (degrees C) value.  If the result is out of normal range,
        // then assume it is malfunctioning and display an error.
        float temp = rRtc.temperature();
        if ((temp <= 0.0) || (temp > 50.0))
        {
            const uint32_t RTC_ERROR = 5;
            printlnE("RTC interface failure.");
            return RTC_ERROR;
        }

        // If the RTC is uninitialized, then set a default time (the start of 2024).
        if (rRtc.oscillatorStopFlag())
        {
            printlnD("RTC uninitialized.");
            rRtc.now(DateTime(2024, 1, 1, 0, 0, 0));
            rRtc.oscillatorStopFlag(false);
        }

        // Setup the RTC callbacks.  These should be done before calling
        // WiFiTimeManager::Init(), since it will use the callbacks to initialize
        // the current time.
        pWtm->SetUtcGetCallback(UtcGetCallback);
        pWtm->SetUtcSetCallback(UtcSetCallback);
        return 0;
    } // End SetupRtc().


    /////////////////////////////////////////////////////////////////////////////
    // UtcGetCallback()
    //
    // This callback is invoked when NTP time is not called, and is used to
    // supply current time_t time.  In this case we read the current time from
    // the RTC, convert it to time_t, and return it.
    /////////////////////////////////////////////////////////////////////////////
    time_t UtcGetCallback()
    {
        DateTime t = gRtc.now();
        return t.get_time_t();
    } // End UtcGetCallback().


    /////////////////////////////////////////////////////////////////////////////
    // UtcSetCallback()
    //
    // This callback is invoked after an NTP time update is received from the NTP
    // server.  It converts the time_t value to a DateTime value and updates the
    // RTC time.
    /////////////////////////////////////////////////////////////////////////////
    void UtcSetCallback(time_t t)
    {
        // Push the new time to the RTC.
        gRtc.now(DateTime(t));

        // Blink LED to show that we just got an update.
        gClock.RgbLed.brightness(RGBLed::MAGENTA, 2);

        // Reset the RTC stop flag to inidcate that the RTC time is valid.
        // Really only needs to be done once, but adds little overhead when
        // done here.
        gRtc.oscillatorStopFlag(false);
    } // End UtcSetCallback().

#endif // End USE_RTC.


/////////////////////////////////////////////////////////////////////////////////
// CheckButton()
//
// This function checks the reset button.  If pressed for a long time (about 3
// seconds), it will reset all of our WiFi credentials as well as all timezone,
// DST, and NTP data, then reset the processor.  If pressed for a short time and
// the network is not connected, it will start the config portal.  It will also
// home the clock.
/////////////////////////////////////////////////////////////////////////////////
void CheckButton()
{
    // Check for a button press.
    if (gClock.IsButtonPressed())
    {
        // Poor mans debounce/press-hold.
        delay(50);
        if(gClock.IsButtonPressed())
        {
            printlnI("Button Pressed.");
            // Loop to check for button release.  If still pressed after 3
            // seconds, indicates a long press (reset request).
            // If released before 3 seconds, indicates a short press which
            // will preform a home, and restart the config portal if not
            // currently connected.
            const uint32_t BUTTON_CHECK_DELAY_MS = 100;
            for (uint32_t i = 0; i < 3000; i += BUTTON_CHECK_DELAY_MS)
            {
                if (!gClock.IsButtonPressed())
                {
                    if (!gpWtm->IsConnected())
                    {
                        printlnI("Starting config portal.");
                        gpWtm->setConfigPortalBlocking(false);
                        gpWtm->setConfigPortalTimeout(0);
                        gpWtm->startConfigPortal(AP_NAME);
                    }
                    gClock.Home();
                    break;
                }
                delay(BUTTON_CHECK_DELAY_MS);
            }
            // Still holding button for 3s, reset settings and restart.
            if(gClock.IsButtonPressed())
            {
                printlnI("Button Held, Erasing Config and restarting.");
                gpWtm->ResetData();
                ESP.restart();
            }
        }
    }
} // End CheckButton().


/////////////////////////////////////////////////////////////////////////////////
// ReportIfError()
//
// This function repeatedly blinks the error (red) LED the specified number of
// times if the specified count is non-zero.  A 'blinkCount' value of zero simply
// returns.
//
// Note that a non-zero 'blinkCount' will cause the function to repeat until
// the pushbotton is pressed.  This will cause the system to reboot.
//
// Arguments:
//    - blinkCount - This is the number of blinks to repeatedly display.  A value
//                   of zero will cause the function to return immediately without
//                   blinking.
//
// Note: Beware that this function enters an (almost) infinite loop.  The only
//       way to exit (if the blinkCount is non-zero) is to press the pushbutton
//       which will cause the clock to reboot.
/////////////////////////////////////////////////////////////////////////////////
void ReportIfError(uint32_t blinkCount)
{
    // If count is non-zero, then repeatedly blink the specified number of times.
    // Otherwise, simply return.
    if (blinkCount)
    {
        // Turn off all LEDs.
        gClock.RgbLed.off();
        gClock.RgbLed.brightness(100);

        // Blink the specified number of times forever!
        while (1)
        {
            delay(2000);

            // Blink the error LED the specified number of times.
            for (int i = 0; i < blinkCount; i++)
            {
                gClock.RgbLed.flash(ERROR_LED, 150, 200);
            }

            // Check for button press.  If so, restart the system.
            if (gClock.IsButtonPressed())
            {
                ESP.restart();
            }
        }
    }
} // End ReportIfError().


/////////////////////////////////////////////////////////////////////////////////
// setup()
//
// The Arduino setup() function.  This function initializes all the hardware.
/////////////////////////////////////////////////////////////////////////////////
void setup()
{
    // Get the Serial class ready for use.
    Serial.begin(250000);
    Serial.setDebugOutput(true);
    delay(1000);
    printlnV("Starting.");

    // If the pushbutton is pressed at startup, then perform a home calibration.
    // The red LED will light when the calibration request is detected.  Release
    // the pushbutton before the red LED goes out (2 seconds) in order for the
    // calibration to begin.
    if (gClock.IsButtonPressed())
    {
        gClock.RgbLed.brightness(RGBLed::WHITE, 2);
        delay(2000);
        gClock.Calibrate();
    }

    // Cycle the LEDs at power up just to show that they work.  Here we do some
    // fancy fading of each LED just to show off (and to verify that dimming works).
    const int FADE_STEPS = 75;
    const int FADE_DURATION_MS = 750;
    gClock.RgbLed.fadeIn(NTP_CLOCK_LED, FADE_STEPS, FADE_DURATION_MS);
    gClock.RgbLed.fadeOut(NTP_CLOCK_LED, FADE_STEPS, FADE_DURATION_MS);
    gClock.RgbLed.fadeIn(LOCAL_CLOCK_LED, FADE_STEPS, FADE_DURATION_MS);
    gClock.RgbLed.fadeOut(LOCAL_CLOCK_LED, FADE_STEPS, FADE_DURATION_MS);
    gClock.RgbLed.fadeIn(ERROR_LED, FADE_STEPS, FADE_DURATION_MS);
    gClock.RgbLed.fadeOut(ERROR_LED, FADE_STEPS, FADE_DURATION_MS);
    gClock.RgbLed.brightness(2);

    // Home the clock to 12:00 while showing white LED.  Display any error.
    gClock.RgbLed.brightness(RGBLed::WHITE, 2);
    ReportIfError(static_cast<uint32_t>(gClock.Home()));
    gClock.RgbLed.off();

    // Init a pointer to our WiFiTimeManager instance.
    // This should be done before RTC init since the WiFiTimeManager
    // gets created on the first call to WiFiTimeManager::Instance() and it
    // initializes a default time that the RTC may want to override.
    gpWtm = WiFiTimeManager::Instance();

#if defined USE_RTC
    // Initialize the RTC and report an error on failure.  This should be done
    // before calling WiFiTimeManager::Init(), since it will use the RTC callbacks
    // to initialize the current time.
    ReportIfError(SetupRtc(gRtc, gpWtm));
#endif // End USE_RTC.

    // Initialize the WiFiTimeManager class with our AP and button selections.
    gpWtm->Init(AP_NAME, AP_PWD, SETUP_BUTTON);

    // Contact the NTP server no more than once per 10 minutes.
    gpWtm->SetMinNtpRateSec(10 * 60);

    // Attempt to connect to the network in non-blocking mode.
    gpWtm->setConfigPortalBlocking(BLOCKING_MODE);
    gpWtm->setConfigPortalTimeout(0);
    if(!gpWtm->autoConnect())
    {
        printlnA("Failed to connect or hit timeout.");
    }
    else
    {
        // If we get here we have connected to the WiFi.
        printlnA("Connected.)");
        gpWtm->GetUtcTimeT();
    }

} // End setup().


/////////////////////////////////////////////////////////////////////////////////
// loop()
//
// The Arduino loop() function.  Polls the WiFiTimeManager if we are not already
// connected to the WiFi.  Get the UTC time on a transition of the WiFi being
// connected.  Re-home the clock twice per day.
/////////////////////////////////////////////////////////////////////////////////
void loop()
{
    // If not connected, check for a new connection.
    if(!gpWtm->IsConnected())
    {
        // Avoid delays() in loop when non-blocking and other long running code.
        if (gpWtm->process())
        {
            // This is the place to do something when we transition from
            // unconnected to connected.  As an example, here we get the time.
            gpWtm->GetUtcTimeT();
        }
    }

    // Update the LEDs.
    gClock.RgbLed.brightness(
        gpWtm->UsingNetworkTime() ? NTP_CLOCK_LED : LOCAL_CLOCK_LED, 2);

    // Update the time and run the clock's mechanics.
    tm now;
    gpWtm->GetLocalTime(&now);
    gClock.UpdateClock(now);

#if defined HOME_AT_12
    // Re adjust the clock twice per day at 12:00 if desired.
    static bool clockAdjusted = false;
    if (((now.tm_hour % 12) == 0) && (now.tm_min == 0))
    {
        // If we haven't done so yet since it turned 12:00:00 , home the clock in
        // order to keep it accurate. This insures that we only home the clock
        // once at each 12:00:00.
        if (!clockAdjusted)
        {
            gClock.Home();
            clockAdjusted = true;
        }
    }
    else
    {
        clockAdjusted = false;
    }
#endif // HOME_AT_12

    // Update the debug handler.
    debugHandle();

    // Read the time every 10 seconds (for debug only).
    static uint32_t lastTime = millis();
    uint32_t thisTime = millis();
    const uint32_t updateTime = 10000;
    if (thisTime - lastTime >= updateTime)
    {
        // Read the local time and display the results.
        lastTime = thisTime;
        gpWtm->PrintDateTime(&now);
    }

    // Add some delay till the next loop iterataion.
    const uint32_t LOOP_DELAY_MS = 100;
    delay(LOOP_DELAY_MS);

} // End loop().
