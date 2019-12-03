#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <QtGlobal>
#include <QMap>

/*!
 * \brief The JOYCON_BUTTONS enum is used to mask
 * button status from the bytes of a standard input report
 * the joycons
 */
 enum JOYCON_BUTTONS : int {
    // byte 3:
    R_BUT_Y = 1,
    R_BUT_X = 2,
    R_BUT_B = 4,
    R_BUT_A = 8,
    R_BUT_SR = 16,
    R_BUT_SL = 32,
    R_BUT_R = 64,
    R_BUT_ZR = 128,
    // byte 4:, & with 256 for key mapping
    L_BUT_MINUS= 1,
    R_BUT_PLUS = 2,
    R_BUT_STICK = 4,
    L_BUT_STICK = 8,
    R_BUT_HOME = 16,
    L_BUT_CAP = 32,
    // 64 is unused
    CHARGE_GRIP = 128,
    // byte 5:, & with 512 for key mapping
    L_BUT_DOWN = 1,
    L_BUT_UP = 2,
    L_BUT_RIGHT = 4,
    L_BUT_LEFT = 8,
    L_BUT_SR = 16,
    L_BUT_SL = 32,
    L_BUT_L = 64,
    L_BUT_ZL = 128
};

class EventHandler
{
public:
    EventHandler();
    void handleMouseMove(double dx, double dy);
    void handleButtonPress(int button_mask);
    void handleButtonRelease(int button_mask);
    void addMapping(int from, int to);
    void mapToMouseButton(int mask, int button);

private:
    //< a map to lookup OS button press from JoyCon buttom press
    //< JoyCon buttons from byte 4 are & with 256
    //< JoyCon buttons from byte 5 are & with 512
    QMap<int,int> keyCodeMap;
    QMap<int, unsigned short> qt_to_cg;
    bool _left_button_down = false;
    bool _right_button_down = false;
};

#endif // EVENTHANDLER_H
