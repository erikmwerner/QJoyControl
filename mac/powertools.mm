#include "powertools.h"
#include <Foundation/Foundation.h> // App Nap API
#import <IOKit/pwr_mgt/IOPMLib.h> // Power Management System API

static id activity; //< keep track of the activity ID for app nap control
static IOPMAssertionID assertionID; //< keep track of the assertion ID for sleep control

/*!
 * \brief disableAppNap
 * \param reason_string a description of the activity preventing app nap. Used for debugging.
 */
void disableAppNap(const QString reason_string) {
    NSString* nss = [[NSString alloc] initWithUTF8String:reason_string.toUtf8().data()];
    activity = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityLatencyCritical | NSActivityUserInitiated
                                                              reason:nss];
}

/*!
 * \brief enableAppNap
 */
void enableAppNap() {
    [[NSProcessInfo processInfo] endActivity:activity];
}

/*!
 * \brief preventSleep
 * \param assert_type the assertion type to request from the power management system
 * Use "PreventUserIdleSystemSleep" to prevent system sleep, but allow display sleeping.
 * Use "PreventUserIdleDisplaySleep" to prevent both system and display sleeping (this is the default)
 * \param reason_string a string describing the activity preventing sleep. Max length is 128 characters
 * \return true if the assertion was successfully activated
 */
bool preventSleep(const QString assert_type, const QString reason_string){
    CFStringRef reason_c = reason_string.toCFString();
    CFStringRef assertion_c = assert_type.toCFString();
    IOReturn success = IOPMAssertionCreateWithName(assertion_c,
                                kIOPMAssertionLevelOn, reason_c, &assertionID);
    if (success == kIOReturnSuccess){
        return true;
    } else {
        return false;
    }
}

/*!
 * \brief allowSleep
 * \return true if the assertio was successfully released
 */
bool allowSleep(){
    IOReturn success = IOPMAssertionRelease(assertionID);
    if (success == kIOReturnSuccess){
        return true;
    } else {
        return false;
    }
}
