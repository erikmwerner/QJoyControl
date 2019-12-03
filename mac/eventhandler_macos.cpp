#include "eventhandler.h"
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

// key code mappings
// https://stackoverflow.com/questions/1918841/how-to-convert-ascii-character-to-cgkeycode/14529841#14529841][1]

/* The CGKeyCodes enum is used by
 * MacOS Core Graphics to represent the virtual
 * key codes used in keyboard events
 */

/*!
 * \brief EventHandler::EventHandler
 */
EventHandler::EventHandler()
{
    // CGKeyCodes from Events.h in carbon
    // based on US Extended keyboard
    qt_to_cg.insert(Qt::Key_A, kVK_ANSI_A);
    qt_to_cg.insert(Qt::Key_S, kVK_ANSI_S);
    qt_to_cg.insert(Qt::Key_D, kVK_ANSI_D);
    qt_to_cg.insert(Qt::Key_F, kVK_ANSI_F);
    qt_to_cg.insert(Qt::Key_H, kVK_ANSI_H);
    qt_to_cg.insert(Qt::Key_G, kVK_ANSI_G);
    qt_to_cg.insert(Qt::Key_Z, kVK_ANSI_Z);
    qt_to_cg.insert(Qt::Key_X, kVK_ANSI_X);
    qt_to_cg.insert(Qt::Key_C, kVK_ANSI_C);
    qt_to_cg.insert(Qt::Key_V, kVK_ANSI_V);
    qt_to_cg.insert(Qt::Key_B, kVK_ANSI_B);
    qt_to_cg.insert(Qt::Key_Q, kVK_ANSI_Q);
    qt_to_cg.insert(Qt::Key_W, kVK_ANSI_W);
    qt_to_cg.insert(Qt::Key_E, kVK_ANSI_E);
    qt_to_cg.insert(Qt::Key_R, kVK_ANSI_R);
    qt_to_cg.insert(Qt::Key_T, kVK_ANSI_T);
    qt_to_cg.insert(Qt::Key_Y, kVK_ANSI_Y);

    qt_to_cg.insert(Qt::Key_1, kVK_ANSI_1);
    qt_to_cg.insert(Qt::Key_2, kVK_ANSI_2);
    qt_to_cg.insert(Qt::Key_3, kVK_ANSI_3);
    qt_to_cg.insert(Qt::Key_4, kVK_ANSI_4);
    qt_to_cg.insert(Qt::Key_6, kVK_ANSI_6);
    qt_to_cg.insert(Qt::Key_5, kVK_ANSI_5);
    qt_to_cg.insert(Qt::Key_Equal, kVK_ANSI_Equal);
    qt_to_cg.insert(Qt::Key_9, kVK_ANSI_9);
    qt_to_cg.insert(Qt::Key_7, kVK_ANSI_7);
    qt_to_cg.insert(Qt::Key_Minus, kVK_ANSI_Minus);
    qt_to_cg.insert(Qt::Key_8, kVK_ANSI_8);
    qt_to_cg.insert(Qt::Key_0, kVK_ANSI_0);
    qt_to_cg.insert(Qt::Key_BracketRight, kVK_ANSI_RightBracket);
    qt_to_cg.insert(Qt::Key_O, kVK_ANSI_O);
    qt_to_cg.insert(Qt::Key_U, kVK_ANSI_U);
    qt_to_cg.insert(Qt::Key_BracketLeft, kVK_ANSI_LeftBracket);
    qt_to_cg.insert(Qt::Key_I, kVK_ANSI_I);
    qt_to_cg.insert(Qt::Key_P, kVK_ANSI_P);
    qt_to_cg.insert(Qt::Key_L, kVK_ANSI_L);
    qt_to_cg.insert(Qt::Key_J, kVK_ANSI_J);
    qt_to_cg.insert(Qt::Key_QuoteLeft, kVK_ANSI_Quote);
    qt_to_cg.insert(Qt::Key_K, kVK_ANSI_K);
    qt_to_cg.insert(Qt::Key_Semicolon, kVK_ANSI_Semicolon);
    qt_to_cg.insert(Qt::Key_Backslash, kVK_ANSI_Backslash);
    qt_to_cg.insert(Qt::Key_Comma, kVK_ANSI_Comma);
    qt_to_cg.insert(Qt::Key_Slash, kVK_ANSI_Slash);
    qt_to_cg.insert(Qt::Key_N, kVK_ANSI_N);
    qt_to_cg.insert(Qt::Key_M, kVK_ANSI_M);
    qt_to_cg.insert(Qt::Key_Period, kVK_ANSI_Period);
    qt_to_cg.insert(Qt::Key_Dead_Grave, kVK_ANSI_Grave);
    // there are more ansi...

    qt_to_cg.insert(Qt::Key_Return, kVK_Return);
    qt_to_cg.insert(Qt::Key_Tab, kVK_Tab);
    qt_to_cg.insert(Qt::Key_Space, kVK_Space);
    qt_to_cg.insert(Qt::Key_Delete, kVK_Delete);
    qt_to_cg.insert(Qt::Key_Escape, kVK_Escape);
    qt_to_cg.insert(Qt::Key_Control, kVK_Command);
    qt_to_cg.insert(Qt::Key_Period, kVK_Shift);
    qt_to_cg.insert(Qt::Key_CapsLock, kVK_CapsLock);
    qt_to_cg.insert(Qt::Key_Option, kVK_Option);
    qt_to_cg.insert(Qt::Key_Meta, kVK_Control);
    //qt_to_cg.insert(Qt::Key_Func kVK_Function);
    qt_to_cg.insert(Qt::Key_F17, kVK_F17);
    qt_to_cg.insert(Qt::Key_VolumeUp, kVK_VolumeUp);
    qt_to_cg.insert(Qt::Key_VolumeDown, kVK_VolumeDown);
    //qt_to_cg.insert(Qt::Key_Mute, kVK_Mute);
    qt_to_cg.insert(Qt::Key_F18, kVK_F18);
    qt_to_cg.insert(Qt::Key_F19, kVK_F19);
    qt_to_cg.insert(Qt::Key_F20, kVK_F20);
    qt_to_cg.insert(Qt::Key_F5, kVK_F5);
    qt_to_cg.insert(Qt::Key_F6, kVK_F6);
    qt_to_cg.insert(Qt::Key_F7, kVK_F7);
    qt_to_cg.insert(Qt::Key_F3, kVK_F3);
    qt_to_cg.insert(Qt::Key_F8, kVK_F8);
    qt_to_cg.insert(Qt::Key_F9, kVK_F9);
    qt_to_cg.insert(Qt::Key_F11, kVK_F11);
    qt_to_cg.insert(Qt::Key_F13, kVK_F13);
    qt_to_cg.insert(Qt::Key_F16, kVK_F16);
    qt_to_cg.insert(Qt::Key_F14, kVK_F14);
    qt_to_cg.insert(Qt::Key_F10, kVK_F10);
    qt_to_cg.insert(Qt::Key_F12, kVK_F12);
    qt_to_cg.insert(Qt::Key_F15, kVK_F15);
    qt_to_cg.insert(Qt::Key_Help, kVK_Help);
    qt_to_cg.insert(Qt::Key_Home, kVK_Home);
    qt_to_cg.insert(Qt::Key_PageUp, kVK_PageUp);
    qt_to_cg.insert(Qt::Key_Forward, kVK_ForwardDelete);
    qt_to_cg.insert(Qt::Key_F4, kVK_F4);
    qt_to_cg.insert(Qt::Key_End, kVK_End);
    qt_to_cg.insert(Qt::Key_F2, kVK_F2);
    qt_to_cg.insert(Qt::Key_PageDown, kVK_PageDown);
    qt_to_cg.insert(Qt::Key_F1, kVK_F1);
    qt_to_cg.insert(Qt::Key_Left, kVK_LeftArrow);
    qt_to_cg.insert(Qt::Key_Right, kVK_RightArrow);
    qt_to_cg.insert(Qt::Key_Down, kVK_DownArrow);
    qt_to_cg.insert(Qt::Key_Up, kVK_UpArrow);
}

void EventHandler::addMapping(int from, int to)
{
    keyCodeMap.insert(from, to);
}

void EventHandler::mapToMouseButton(int mask, int button)
{
    if(button == 1) {
        addMapping(mask, kCGEventRightMouseDown);
    }
    else if (button == 2) {
        addMapping(mask, kCGEventLeftMouseDown);
    }
}

void EventHandler::handleMouseMove(double dx, double dy){
    CGEventRef get = CGEventCreate(nullptr);
    CGPoint mouse = CGEventGetLocation(get);

    CGPoint pos;
    pos.x = mouse.x + dx;
    pos.y = mouse.y + dy;

    // see https://github.com/debauchee/barrier/blob/master/src/lib/platform/OSXScreen.mm
    // check if cursor position is valid on the client display configuration
    // stkamp@users.sourceforge.net
    CGDisplayCount displayCount = 0;
    CGGetDisplaysWithPoint(pos, 0, nullptr, &displayCount);
    if (displayCount == 0) {
        // cursor position invalid - clamp to bounds of last valid display.
        // find the last valid display using the last cursor position.
        displayCount = 0;
        CGDirectDisplayID displayID;
        CGGetDisplaysWithPoint(CGPointMake(mouse.x, mouse.y), 1,
                               &displayID, &displayCount);
        if (displayCount != 0) {
            CGRect displayRect = CGDisplayBounds(displayID);
            if (pos.x < displayRect.origin.x) {
                pos.x = displayRect.origin.x;
            }
            else if (pos.x > displayRect.origin.x +
                     displayRect.size.width - 1) {
                pos.x = displayRect.origin.x + displayRect.size.width - 1;
            }
            if (pos.y < displayRect.origin.y) {
                pos.y = displayRect.origin.y;
            }
            else if (pos.y > displayRect.origin.y +
                     displayRect.size.height - 1) {
                pos.y = displayRect.origin.y + displayRect.size.height - 1;
            }
        }
    }
    CGEventType event_type = kCGEventMouseMoved;
    CGMouseButton button = kCGMouseButtonLeft;
    if(_left_button_down) {
        event_type = kCGEventLeftMouseDragged;
    }
    if(_right_button_down) {
        event_type = kCGEventRightMouseDragged;
        button = kCGMouseButtonRight;
    }
    CGEventRef mouseEv = CGEventCreateMouseEvent(
                nullptr, event_type,
                pos, button);

    double final_dx = pos.x - mouse.x;
    double final_dy = pos.y - mouse.y;

    CGEventSetIntegerValueField(mouseEv, kCGMouseEventDeltaX, (int)final_dx);
    CGEventSetIntegerValueField(mouseEv, kCGMouseEventDeltaY, (int)final_dy);

    CGEventSetDoubleValueField(mouseEv, kCGMouseEventDeltaX, final_dx);
    CGEventSetDoubleValueField(mouseEv, kCGMouseEventDeltaY, final_dy);

    CGEventPost(kCGHIDEventTap, mouseEv);
    CFRelease(mouseEv);
}

/*!
 * \brief EventHandler::handleButtonPress
 * \param button_mask
 */
void EventHandler::handleButtonPress(int button_mask){
    // check if a mouse button is mapped to the button
    if(keyCodeMap.value(button_mask) == kCGEventLeftMouseDown){

        CGEventRef get = CGEventCreate(nullptr);
        CGPoint mouse = CGEventGetLocation(get);
        int64_t x = mouse.x;
        int64_t y = mouse.y;

        CGEventRef mouseEv1 = CGEventCreateMouseEvent(
                    nullptr, kCGEventLeftMouseDown,
                    CGPointMake(x, y),
                    kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, mouseEv1);
        CFRelease(mouseEv1);
        _left_button_down = true;
    }
    else if(keyCodeMap.value(button_mask) == kCGEventRightMouseDown){

        CGEventRef get = CGEventCreate(nullptr);
        CGPoint mouse = CGEventGetLocation(get);
        int64_t x = mouse.x;
        int64_t y = mouse.y;

        CGEventRef mouseEv1 = CGEventCreateMouseEvent(
                    nullptr, kCGEventRightMouseDown,
                    CGPointMake(x, y),
                    kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, mouseEv1);
        CFRelease(mouseEv1);
        _right_button_down = true;
    }
    // otherwise, get the key code value for the button
    else {
        int qt_key = keyCodeMap.value(button_mask,  Qt::Key_Enter);
        unsigned short cg_key = qt_to_cg.value(qt_key);
        CGEventRef keyEvent = CGEventCreateKeyboardEvent( nullptr, cg_key, true ) ;
        CGEventPost( kCGHIDEventTap, keyEvent ) ;
        CFRelease( keyEvent ) ;
    }
}

void EventHandler::handleButtonRelease(int button_mask){
    if(keyCodeMap.value(button_mask) == kCGEventLeftMouseDown ){

        CGEventRef get = CGEventCreate(nullptr);
        CGPoint mouse = CGEventGetLocation(get);
        int64_t x = mouse.x;
        int64_t y = mouse.y;

        CGEventRef mouseEv2 = CGEventCreateMouseEvent(
                    nullptr, kCGEventLeftMouseUp,
                    CGPointMake(x, y),
                    kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, mouseEv2);
        CFRelease(mouseEv2);
        _left_button_down = false;
    }
    if(keyCodeMap.value(button_mask) == kCGEventRightMouseDown ){

        CGEventRef get = CGEventCreate(nullptr);
        CGPoint mouse = CGEventGetLocation(get);
        int64_t x = mouse.x;
        int64_t y = mouse.y;

        CGEventRef mouseEv2 = CGEventCreateMouseEvent(
                    nullptr, kCGEventRightMouseUp,
                    CGPointMake(x, y),
                    kCGMouseButtonLeft);
        CGEventPost(kCGHIDEventTap, mouseEv2);
        CFRelease(mouseEv2);
        _right_button_down = false;
    }
    else {
        int qt_key = keyCodeMap.value(button_mask, Qt::Key_Enter);
        unsigned short cg_key = qt_to_cg.value(qt_key);
        CGEventRef keyEvent = CGEventCreateKeyboardEvent( nullptr, cg_key, false ) ;
        CGEventPost( kCGHIDEventTap, keyEvent ) ;
        CFRelease( keyEvent ) ;
    }
}
