#include "joyconworker.h"
#include "hidapi.h"
#include "luts.h"
#include "ir_sensor.h"

#include <QDateTime>
#include <QThread>
#include <cmath>

#include <qdebug.h>

/******************** pointer type casting functions

int16_t uint16_to_int16(uint16_t a) {
    int16_t b;
    char* aPointer = (char*)&a, *bPointer = (char*)&b;
    memcpy(bPointer, aPointer, sizeof(a));
    return b;
}

uint16_t int16_to_uint16(int16_t a) {
    uint16_t b;
    char* aPointer = (char*)&a, *bPointer = (char*)&b;
    memcpy(bPointer, aPointer, sizeof(a));
    return b;
}
*/


void JoyConWorker::requestDeviceInfo(){
    unsigned char device_info[10];
    memset(device_info, 0, sizeof(device_info));
    get_device_info(device_info);
    QList<unsigned char> info;
    for(int i = 0; i<10; ++i){
        info.append(device_info[i]);
    }
    emit newDeviceInfo(info, _joycon_type);
}

void JoyConWorker::requestTemperature(){
    unsigned char temp_info[2];
    memset(temp_info, 0, sizeof(temp_info));
    if(get_temperature(temp_info) != 0) {
        return;
    }
    // Convert data to Celsius
    float temp_c = 25.0f + static_cast<int16_t>(temp_info[1] << 8 | temp_info[0]) * 0.0625f;

    emit newTemperatureData(temp_c);
}

void JoyConWorker::requestBattery(){
    unsigned char batt_info[3];
    memset(batt_info, 0, sizeof(batt_info));

    if(get_battery(batt_info) != 0) {
        return;
    }

    int batt_percent = 0;
    // int batt = ((uint8_t)batt_info[0] & 0xF0) >> 4;
    // get battery status
    int status = static_cast<uint8_t>(batt_info[0] & 0xF0) >> 4;

    // Calculate aproximate battery percent from regulated voltage
    uint16_t batt_volt = (uint8_t)batt_info[1] + ((uint8_t)batt_info[2] << 8);
    if (batt_volt < 0x560)
        batt_percent = 1;
    else if (batt_volt > 0x55F && batt_volt < 0x5A0) {
        batt_percent = ((batt_volt - 0x60) & 0xFF) / 7.0f + 1;
    }
    else if (batt_volt > 0x59F && batt_volt < 0x5E0) {
        batt_percent = ((batt_volt - 0xA0) & 0xFF) / 2.625f + 11;
    }
    else if (batt_volt > 0x5DF && batt_volt < 0x618) {
        batt_percent = (batt_volt - 0x5E0) / 1.8965f + 36;
    }
    else if (batt_volt > 0x617 && batt_volt < 0x658) {
        batt_percent = ((batt_volt - 0x18) & 0xFF) / 1.8529f + 66;
    }
    else if (batt_volt > 0x657)
        batt_percent = 100;

    emit newBatteryData(static_cast<int>(batt_volt), batt_percent, status);
}




/******************** decodes joystick positions?
void decode_stick_params(uint16_t *decoded_stick_params, uint8_t *encoded_stick_params) {
    decoded_stick_params[0] = ((encoded_stick_params[1] << 8) & 0xF00) | encoded_stick_params[0];
    decoded_stick_params[1] = (encoded_stick_params[2] << 4) | (encoded_stick_params[1] >> 4);
}

void encode_stick_params(uint8_t *encoded_stick_params, uint16_t *decoded_stick_params) {
    encoded_stick_params[0] =  decoded_stick_params[0] & 0xFF;
    encoded_stick_params[1] = (decoded_stick_params[0] & 0xF00) >> 8 | (decoded_stick_params[1] & 0xF) << 4;
    encoded_stick_params[2] = (decoded_stick_params[1] & 0xFF0) >> 4;
}
*/

/******************** crc 8 decoding function
 * */
uint8_t mcu_crc8_calc(uint8_t *buf, uint8_t size) {
    uint8_t crc8 = 0x0;

    for (int i = 0; i < size; ++i) {
        crc8 = mcu_crc8_table[(uint8_t)(crc8 ^ buf[i])];
    }
    return crc8;
}

// Credit to Hypersect (Ryan Juckett)
// http://blog.hypersect.com/interpreting-analog-sticks/
void AnalogStickCalc(
        float *pOutX,       // out: resulting stick X value
        float *pOutY,       // out: resulting stick Y value
        uint16_t x,              // in: initial stick X value
        uint16_t y,              // in: initial stick Y value
        uint16_t x_calc[3],      // calc -X, CenterX, +X
uint16_t y_calc[3]       // calc -Y, CenterY, +Y
)
{
    float x_f, y_f;
    // Apply Joy-Con center deadzone. 0xAE translates approx to 15%. Pro controller has a 10% deadzone.
    float deadZoneCenter = 0.15f;
    // Add a small ammount of outer deadzone to avoid edge cases or machine variety.
    float deadZoneOuter = 0.10f;

    // convert to float based on calibration and valid ranges per +/-axis
    x = CLAMP(x, x_calc[0], x_calc[2]);
    y = CLAMP(y, y_calc[0], y_calc[2]);
    if (x >= x_calc[1])
        x_f = (float)(x - x_calc[1]) / (float)(x_calc[2] - x_calc[1]);
    else
        x_f = -((float)(x - x_calc[1]) / (float)(x_calc[0] - x_calc[1]));
    if (y >= y_calc[1])
        y_f = (float)(y - y_calc[1]) / (float)(y_calc[2] - y_calc[1]);
    else
        y_f = -((float)(y - y_calc[1]) / (float)(y_calc[0] - y_calc[1]));

    // Interpolate zone between deadzones
    float mag = sqrtf(x_f*x_f + y_f*y_f);
    if (mag > deadZoneCenter) {
        // scale such that output magnitude is in the range [0.0f, 1.0f]
        float legalRange = 1.0f - deadZoneOuter - deadZoneCenter;
        float normalizedMag = fmin(1.0f, (mag - deadZoneCenter) / legalRange);
        float scale = normalizedMag / mag;
        pOutX[0] = x_f * scale;
        pOutY[0] = y_f * scale;
    }
    else
    {
        // stick is in the inner dead zone
        pOutX[0] = 0.0f;
        pOutY[0] = 0.0f;
    }
}

int JoyConWorker::writePacket(uint8_t *buf, unsigned long len, uint8_t cmd, uint8_t subcmd, uint8_t arg1, uint8_t arg2)
{
    if(handle == nullptr) {return -1;}

    memset(buf, 0, len);
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = cmd;
    hdr->timer = _timing_byte & 0xF;
    //rumble L[4]?
    //rumble R[4]?
    _timing_byte++;
    if(subcmd > 0) {
        pkt->subcmd = subcmd;
    }
    if(arg1 > 0) {
        pkt->subcmd_arg.arg1 = arg1;
    }
    if(arg2 > 0) {
        pkt->subcmd_arg.arg2 = arg2;
    }

    return hid_write(handle, buf, len);
    // res = hid_read_timeout(handle, buf, 0, 64);
}

int JoyConWorker::set_led_busy() {

    int res;
    uint8_t buf[49];
    //res = makePacket(buf, 49, RUMBLE_PLUS, SET_PLAYER_LED, 0x81);
    // flash all 4 player LEDs (1111 0000)
    res = writePacket(buf, 49, RUMBLE_PLUS, SET_PLAYER_LED, 0xF0);
    res = hid_read_timeout(handle, buf, sizeof(buf), 64);

    /*uint8_t buf[49];
    memset(buf, 0, sizeof(buf));
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = 0x01;
    hdr->timer = timming_byte & 0xF;
    timming_byte++;
    pkt->subcmd = 0x30;
    pkt->subcmd_arg.arg1 = 0x81;
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf, 0, 64);*/

    //Set breathing HOME Led
    if (_joycon_type != JOYCON_L) {
        uint8_t buf[49];
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        memset(buf, 0, sizeof(buf));
        hdr = (brcm_hdr *)buf;
        pkt = (brcm_cmd_01 *)(hdr + 1); // why do this twice?
        hdr->cmd = 0x01;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x38;
        pkt->subcmd_arg.arg1 = 0x28;
        //pkt->subcmd_arg.arg2 = 0x20; // flash forever
        pkt->subcmd_arg.arg2 = 0x23; // flash 3 times, start intensity of 12.5%
        buf[13] = 0xF2;
        buf[14] = buf[15] = 0xF0;
        res = hid_write(handle, buf, sizeof(buf));
        res = hid_read_timeout(handle, buf, sizeof(buf), 64);
    }

    return 0;
}

std::string JoyConWorker::get_sn(uint32_t offset, const uint16_t read_len) {
    int res;
    int error_reading = 0;
    uint8_t buf[49];
    std::string test = "";
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 1;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x10;
        pkt->spi_data.offset = offset;
        pkt->spi_data.size = read_len;
        res = hid_write(handle, buf, sizeof(buf));

        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if ((*(uint16_t*)&buf[0xD] == 0x1090) && (*(uint32_t*)&buf[0xF] == offset))
                goto check_result;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 20)
            return "Error!";
    }
check_result:
    if (res >= 0x14 + read_len) {
        for (int i = 0; i < read_len; i++) {
            if (buf[0x14 + i] != 0x000) {
                test += buf[0x14 + i];
            }else
                test += "";
        }
    }
    else {
        return "Error!";
    }
    return test;
}

int JoyConWorker::get_spi_data(uint32_t offset, const uint16_t read_len, uint8_t *test_buf) {
    int res;
    uint8_t buf[49];
    int error_reading = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 1;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x10;
        pkt->spi_data.offset = offset;
        pkt->spi_data.size = read_len;
        res = hid_write(handle, buf, sizeof(buf));

        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if ((*(uint16_t*)&buf[0xD] == 0x1090) && (*(uint32_t*)&buf[0xF] == offset))
                goto check_result;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        qDebug()<<"error reading spi data;";
        if (error_reading > 20) {
            return 1;
        }
    }
check_result:
    if (res >= 0x14 + read_len) {
        for (int i = 0; i < read_len; i++) {
            test_buf[i] = buf[0x14 + i];
        }
    }

    return 0;
}


int JoyConWorker::write_spi_data(uint32_t offset, const uint16_t write_len, uint8_t* test_buf) {
    int res;
    uint8_t buf[49];
    int error_writing = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 1;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x11;
        pkt->spi_data.offset = offset;
        pkt->spi_data.size = write_len;
        for (int i = 0; i < write_len; i++)
            buf[0x10 + i] = test_buf[i];

        res = hid_write(handle, buf, sizeof(buf));
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x1180)
                goto check_result;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_writing++;
        qDebug()<<"error writing spi data;";
        if (error_writing == 20) {
            return 1;
        }
    }
check_result:
    return 0;
}


int JoyConWorker::get_device_info(uint8_t* test_buf) {
    int res;
    uint8_t buf[49];
    int error_reading = 0;
    while (1) {
        res = writePacket(buf, 49, 1, 0x02);
        /*memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 1;
        hdr->timer = timming_byte & 0xF;
        timming_byte++;
        pkt->subcmd = 0x02;
        res = hid_write(handle, buf, sizeof(buf));*/
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x0282)
                goto check_result;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 20)
            break;
    }
check_result:
    for (int i = 0; i < 0xA; i++) {
        test_buf[i] = buf[0xF + i];
    }

    return 0;
}


int JoyConWorker::get_battery(uint8_t* test_buf) {
    int res;
    uint8_t buf[49];
    int error_reading = 0;
    while (1) {
        res = writePacket(buf, 49, RUMBLE_PLUS, GET_VREG);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x50D0)
            {
                test_buf[0] = buf[0x2];
                test_buf[1] = buf[0xF];
                test_buf[2] = buf[0x10];
                return 0;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 20)
            break;
    }
    return -1;
}


/*!
 * \brief JoyConWorker::get_temperature requests a temperature measurement
 * from the JoyCon and stores the data in buf_temperature
 * \param buf_temperature
 * \return
 */
int JoyConWorker::get_temperature(uint8_t* buf_temperature) {
    int res;
    uint8_t buf[49];
    int error_reading = 0;
    bool imu_changed = false; //< track if the IMU had to be turned on

    while (1) {
        res = writePacket(buf, 49, RUMBLE_PLUS, READ_IMU, 0x10, 0x01);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x43C0)
                goto check_result;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 20)
            break;
    }
check_result:
    // enable the IMU if it is not on
    if ((buf[0x11] >> 4) == 0x0) {
        qDebug()<<"enabling imu";
        memset(buf, 0, sizeof(buf));

        res = writePacket(buf, 49, RUMBLE_PLUS, ENABLE_IMU, 0x01);
        res = hid_read_timeout(handle, buf, sizeof(buf), 64);

        imu_changed = true;

        // Let temperature sensor stabilize for a little bit.
        this->thread()->msleep(64);
    }
    error_reading = 0;

    while (1) {

        res = writePacket(buf, 49, RUMBLE_PLUS, READ_IMU, 0x20, 0x02);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x43C0) {
                goto check_result2;
            }

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 20)
            break;
    }
check_result2:
    buf_temperature[0] = buf[0x11];
    buf_temperature[1] = buf[0x12];

    // if the IMU was enabled, turn it off
    if (imu_changed) {
        qDebug()<<"turning off imu";
        res = writePacket(buf, 49, RUMBLE_PLUS, ENABLE_IMU, 0x00);
        res = hid_read_timeout(handle, buf, sizeof(buf), 64);
    }

    return 0;
}

int JoyConWorker::send_rumble() {
    int res;
    uint8_t buf[49];
    uint8_t buf2[49];

    //Enable Vibration
    memset(buf, 0, sizeof(buf));
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = RUMBLE_PLUS;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    pkt->subcmd = ENABLE_VIB;
    pkt->subcmd_arg.arg1 = 0x01; // turn on rumble
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf2, sizeof(buf2), 64);

    //New vibration like switch
    this->thread()->msleep(16);
    //Send confirmation
    memset(buf, 0, sizeof(buf));
    hdr->cmd = RUMBLE_PLUS;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    hdr->rumble_l[0] = 0xc2;
    hdr->rumble_l[1] = 0xc8;
    hdr->rumble_l[2] = 0x03;
    hdr->rumble_l[3] = 0x72;
    memcpy(hdr->rumble_r, hdr->rumble_l, sizeof(hdr->rumble_l));
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf2, sizeof(buf2), 64);

    this->thread()->msleep(81);

    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    hdr->rumble_l[0] = 0x00;
    hdr->rumble_l[1] = 0x01;
    hdr->rumble_l[2] = 0x40;
    hdr->rumble_l[3] = 0x40;
    memcpy(hdr->rumble_r, hdr->rumble_l, sizeof(hdr->rumble_l));
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf2, sizeof(buf2), 64);

    this->thread()->msleep(5);

    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    hdr->rumble_l[0] = 0xc3;
    hdr->rumble_l[1] = 0xc8;
    hdr->rumble_l[2] = 0x60;
    hdr->rumble_l[3] = 0x64;
    memcpy(hdr->rumble_r, hdr->rumble_l, sizeof(hdr->rumble_l));
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf2, sizeof(buf2), 64);

    this->thread()->msleep(5);

    //Disable vibration
    memset(buf, 0, sizeof(buf));
    hdr = (brcm_hdr *)buf;
    pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = RUMBLE_PLUS;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    hdr->rumble_l[0] = 0x00;
    hdr->rumble_l[1] = 0x01;
    hdr->rumble_l[2] = 0x40;
    hdr->rumble_l[3] = 0x40;
    memcpy(hdr->rumble_r, hdr->rumble_l, sizeof(hdr->rumble_l));
    pkt->subcmd = ENABLE_VIB;
    pkt->subcmd_arg.arg1 = 0x00; // turn off rumble
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf, sizeof(buf), 64);


    // set player light to player 1
    memset(buf, 0, sizeof(buf));
    hdr = (brcm_hdr *)buf;
    pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = RUMBLE_PLUS;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    pkt->subcmd = SET_PLAYER_LED;
    pkt->subcmd_arg.arg1 = 0x01; // 0000 0001
    res = hid_write(handle, buf, sizeof(buf));
    res = hid_read_timeout(handle, buf, sizeof(buf), 64);

    /*
    // Set HOME Led
    if (joycon_type != 1) {
        memset(buf, 0, sizeof(buf));
        hdr = (brcm_hdr *)buf;
        pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 0x01;
        hdr->timer = timming_byte & 0xF;
        timming_byte++;
        pkt->subcmd = 0x38;
        // Heartbeat style configuration
        buf[11] = 0xF1;
        buf[12] = 0x00;
        buf[13] = buf[14] = buf[15] = buf[16] = buf[17] = buf[18] = 0xF0;
        buf[19] = buf[22] = buf[25] = buf[28] = buf[31] = 0x00;
        buf[20] = buf[21] = buf[23] = buf[24] = buf[26] = buf[27] = buf[29] = buf[30] = buf[32] = buf[33] = 0xFF;
        res = hid_write(handle, buf, sizeof(buf));
        res = hid_read_timeout(handle, buf, 0, 64);
    }
    */

    return 0;
}



JoyConWorker::JoyConWorker(QObject *parent) : QObject(parent)
{

}

void JoyConWorker::onConnectHID(unsigned short vendor_id,
                                unsigned short product_id,
                                const wchar_t *serial_number)
{
    qDebug()<<"on connect HID";
    if (hid_init() == -1) {
        qDebug()<<"HID init error";
        return;
    }
    qDebug()<<"Opening HID device w/ VID:"<<vendor_id<< " PID:"<<product_id
           << " SN:" <<serial_number<<"SN(string):"<<QString::fromWCharArray(serial_number);

    if(vendor_id != NINTENDO) {
        emit deviceConnectionChanged(QString(), 0);
        emit deviceStatusMessage("Error: Not a nintendo device");
        return;
    }

    handle = hid_open(vendor_id, product_id, serial_number);

    if ( handle ) {
        _joycon_type = product_id;

        wchar_t wstr1[256];
        wchar_t wstr2[256];
        wchar_t wstr3[256];

        memset(wstr1, 0, sizeof(wstr1)/sizeof(wchar_t));
        memset(wstr2, 0, sizeof(wstr2)/sizeof(wchar_t));
        memset(wstr3, 0, sizeof(wstr3)/sizeof(wchar_t));


        // Read the Manufacturer String
        hid_get_manufacturer_string(handle, wstr1, 256);
        // Read the Product String
        hid_get_product_string(handle, wstr2, 256);
        // Read the Serial Number String
        hid_get_serial_number_string(handle, wstr3, 256);
        qDebug()<<"HIDAPI connected to product:"<<QString::fromWCharArray(wstr2) <<"Mfg:"<< QString::fromWCharArray(wstr1)<< "SN:"<< QString::fromWCharArray(wstr3);



        emit deviceConnectionChanged(QString::fromWCharArray(wstr3), _joycon_type);
        emit deviceStatusMessage("JoyCon Connected");

        requestDeviceInfo();
        requestBattery();
        requestTemperature();

        // request battery and temp updates every 60 seconds
        _device_status_timer->start(60000);
    }
    else {
        emit deviceConnectionChanged(QString(), 0);
        emit deviceStatusMessage("Error connecting to device: " +QString::fromWCharArray(serial_number));
        _joycon_type = -1;
        qDebug()<<"Error:"<<hid_error(handle);
    }
}

JoyConWorker::~JoyConWorker()
{
    onDisconnectHID();
    emit finished();
}

void JoyConWorker::onDeviceStatusTimerTimeout()
{
    if(_joycon_type > -1) {
        requestBattery();
        requestTemperature();
    }
}


// create thread objects in setup
void JoyConWorker::setup()
{
    _input_poll_timer = new QTimer(this);
    _input_poll_timer->setTimerType(Qt::PreciseTimer);
    connect(_input_poll_timer, SIGNAL(timeout()),
            this, SLOT(onInputPollTimerTimeout()));
    _device_status_timer = new QTimer(this);
    connect(_device_status_timer, SIGNAL(timeout()),
            this, SLOT(onDeviceStatusTimerTimeout()));
    _irCaptureTimer = new QTimer(this);
    connect(_irCaptureTimer, SIGNAL(timeout()),
            this, SLOT(onIrCaptureTimerTimeout()));
    this->thread()->setPriority(QThread::TimeCriticalPriority);
}

void JoyConWorker::onDisconnectHID()
{
    if(_joycon_type > 0) {

        qDebug()<<"disconnecting...";
        onInputStreamingEnabled(false);

        uint8_t buf[49];
        int res = writePacket(buf, 49, RUMBLE_PLUS, SET_HCI_STATE, 0x00);
        res = hid_read_timeout(handle, buf, sizeof(buf), 400);
        /*
        //_stream_buttons = false;
        unsigned char custom_cmd[7];
        memset(custom_cmd, 0, 7);
        custom_cmd[0] = RUMBLE_PLUS; // cmd
        custom_cmd[5] = SET_HCI_STATE; // subcmd Set HCI state
        custom_cmd[6] = 0x00; // subcmd arg, disconnect and enter sleep mode
        send_custom_command(custom_cmd);*/

        //this->thread()->msleep(1000);

        hid_close(handle);
        handle = nullptr;
        _joycon_type = -1;
        emit deviceConnectionChanged(QString(), 0);
        qDebug()<<"JoyCon Disconnected";
        emit deviceStatusMessage("JoyCon Disconnected");
    }
}

void JoyConWorker::onInputStreamingEnabled(bool enabled) {
    _stream_buttons = enabled;
    if(_stream_buttons) {
        requestJoyConInputs();
        //stream data at (up to) 60Hz (joy con will report evert 15 ms)
        _input_poll_timer->start(15);
    }
    else {
        _input_poll_timer->stop();

        // end streaming
        uint8_t buf[49];
        int res = writePacket(buf, 49, RUMBLE_PLUS, SET_INPUT_MODE, 0x3F);
        res = hid_read_timeout(handle, buf, sizeof(buf), 64);
        res - writePacket(buf, 49, RUMBLE_PLUS, ENABLE_IMU, 0x00);
        res = hid_read_timeout(handle, buf, sizeof(buf), 64);
    }
}

/*!
 * \brief JoyConWorker::onPollingTimerTimeout checcks the device
 * for new standard input data
 */
void JoyConWorker::onInputPollTimerTimeout()
{
    uint8_t buf_reply[0x170]; // 368 byte rx buffer

    int res  = hid_read_timeout(handle, buf_reply, sizeof(buf_reply), 200);

    if (res > 12) {
        if (buf_reply[0] == 0x21 || buf_reply[0] == 0x30 || buf_reply[0] == 0x31 || buf_reply[0] == 0x32 || buf_reply[0] == 0x33) {

            QList<int> button_data = { (int)buf_reply[3], (int)buf_reply[4], (int)buf_reply[5] };
            // emit newButtonData(button_data);

            float cal_x[1] = { 0.0f };
            float cal_y[1] = { 0.0f };

            QList<double> analog_data;

            if (_joycon_type != JOYCON_R) { //read left stick data
                analog_data.append ( (int)( buf_reply[6] | (uint16_t)((buf_reply[7] & 0xF) << 8)) );
                analog_data.append( (int)( (buf_reply[7] >> 4) | (buf_reply[8] << 4)) );

                AnalogStickCalc(
                            cal_x, cal_y,
                            buf_reply[6] | (uint16_t)((buf_reply[7] & 0xF) << 8),
                        (buf_reply[7] >> 4) | (buf_reply[8] << 4),
                        stick_cal_x_l,
                        stick_cal_y_l);

                analog_data.append(cal_x[0]);
                analog_data.append(cal_y[0]);
            }
            else {
                // add place-holder zeros
                analog_data.append(0);
                analog_data.append(0);
                analog_data.append(0);
                analog_data.append(0);
            }
            if (_joycon_type != JOYCON_L) { // read right stick data
                analog_data.append( (int)(buf_reply[9] | (uint16_t)((buf_reply[10] & 0xF) << 8)));
                analog_data.append( (int)((buf_reply[10] >> 4) | (buf_reply[11] << 4)));

                AnalogStickCalc(
                            cal_x, cal_y,
                            buf_reply[9] | (uint16_t)((buf_reply[10] & 0xF) << 8),
                        (buf_reply[10] >> 4) | (buf_reply[11] << 4),
                        stick_cal_x_r,
                        stick_cal_y_r);
                analog_data.append(cal_x[0]);
                analog_data.append(cal_y[0]);
            }
            else {
                // add place-holder zeros
                analog_data.append(0);
                analog_data.append(0);
                analog_data.append(0);
                analog_data.append(0);
            }

            QList<double> accel_data, gyro_data;

            //accel_data.append((float) (static_cast<int16_t>(buf_reply[13]) | static_cast<int16_t>((buf_reply[14] << 8) & 0xFF00)) * acc_cal_coeff[0]);
            //accel_data.append((float) (static_cast<int16_t>(buf_reply[15]) | static_cast<int16_t>((buf_reply[16] << 8) & 0xFF00)) * acc_cal_coeff[1]);
            //accel_data.append((float) (static_cast<int16_t>(buf_reply[17]) | static_cast<int16_t>((buf_reply[18] << 8) & 0xFF00)) * acc_cal_coeff[2]);
            //accel_data.append((float)(uint16_to_int16(buf_reply[13] | ((buf_reply[14] << 8) & 0xFF00))) * acc_cal_coeff[0]);
            //accel_data.append((float)(uint16_to_int16(buf_reply[15] | ((buf_reply[16] << 8) & 0xFF00))) * acc_cal_coeff[1]);
            //accel_data.append((float)(uint16_to_int16(buf_reply[17] | ((buf_reply[18] << 8) & 0xFF00)))  * acc_cal_coeff[2]);
            analog_data.append((float)(static_cast<int16_t>(buf_reply[13] | ((buf_reply[14] << 8) & 0xFF00))) * acc_cal_coeff[0]);
            analog_data.append((float)(static_cast<int16_t>(buf_reply[15] | ((buf_reply[16] << 8) & 0xFF00))) * acc_cal_coeff[1]);
            analog_data.append((float)(static_cast<int16_t>(buf_reply[17] | ((buf_reply[18] << 8) & 0xFF00)))  * acc_cal_coeff[2]);

            //gyro_data.append((float) (static_cast<int16_t>(buf_reply[19]) | static_cast<int16_t>((buf_reply[20] << 8) & 0xFF00)) * gyro_cal_coeff[0]);
            //gyro_data.append((float) (static_cast<int16_t>(buf_reply[21]) | static_cast<int16_t>((buf_reply[22] << 8) & 0xFF00)) * gyro_cal_coeff[1]);
            //gyro_data.append((float) (static_cast<int16_t>(buf_reply[23]) | static_cast<int16_t>((buf_reply[24] << 8) & 0xFF00)) * gyro_cal_coeff[2]);

            //gyro_data.append((float)(uint16_to_int16(buf_reply[19] | ((buf_reply[20] << 8) & 0xFF00)) - uint16_to_int16(sensor_cal[1][0])) * gyro_cal_coeff[0]);
            //gyro_data.append((float)(uint16_to_int16(buf_reply[21] | ((buf_reply[22] << 8) & 0xFF00)) - uint16_to_int16(sensor_cal[1][1])) * gyro_cal_coeff[1]);
            //gyro_data.append((float)(uint16_to_int16(buf_reply[23] | ((buf_reply[24] << 8) & 0xFF00)) - uint16_to_int16(sensor_cal[1][2])) * gyro_cal_coeff[2]);

            analog_data.append((float)(static_cast<int16_t>(buf_reply[19] | ((buf_reply[20] << 8) & 0xFF00)) - static_cast<int16_t>(sensor_cal[1][0])) * gyro_cal_coeff[0]);
            analog_data.append((float)(static_cast<int16_t>(buf_reply[21] | ((buf_reply[22] << 8) & 0xFF00)) - static_cast<int16_t>(sensor_cal[1][1])) * gyro_cal_coeff[1]);
            analog_data.append((float)(static_cast<int16_t>(buf_reply[23] | ((buf_reply[24] << 8) & 0xFF00)) - static_cast<int16_t>(sensor_cal[1][2])) * gyro_cal_coeff[2]);

            // send inputs as a list of ints and a list of doubles
            emit newInputData(button_data, analog_data);
        }
    } // if(res > 12)
}

void JoyConWorker::requestJoyConInputs(){

    set_led_busy();
    send_rumble();

    this->thread()->msleep(15);

    // tk check how these are used
    // flags to track if the user has custom calibraions
    // bool has_user_cal_stick_l = false;
    // bool has_user_cal_stick_r = false;
    // bool has_user_cal_sensor = false

    memset(factory_stick_cal, 0, 0x12);
    memset(user_stick_cal, 0, 0x16);
    memset(sensor_model, 0, 0x6);
    memset(stick_model, 0, 0x12);
    memset(factory_sensor_cal, 0, 0x18);
    memset(user_sensor_cal, 0, 0x1A);
    memset(sensor_cal, 0, sizeof(sensor_cal));
    memset(stick_cal_x_l, 0, sizeof(stick_cal_x_l));
    memset(stick_cal_y_l, 0, sizeof(stick_cal_y_l));
    memset(stick_cal_x_r, 0, sizeof(stick_cal_x_r));
    memset(stick_cal_y_r, 0, sizeof(stick_cal_y_r));

    get_spi_data(0x6020, 0x18, factory_sensor_cal);
    get_spi_data(0x603D, 0x12, factory_stick_cal);
    get_spi_data(0x6080, 0x6, sensor_model);
    get_spi_data(0x6086, 0x12, stick_model);
    get_spi_data(0x6098, 0x12, &stick_model[0x12]);
    get_spi_data(0x8010, 0x16, user_stick_cal);
    get_spi_data(0x8026, 0x1A, user_sensor_cal);


    // Analog Stick device parameters
    // Stick calibration
    if (_joycon_type != 2) {
        stick_cal_x_l[1] = ((factory_stick_cal[4] << 8) & 0xF00) | factory_stick_cal[3];
        stick_cal_y_l[1] = (factory_stick_cal[5] << 4) | (factory_stick_cal[4] >> 4);
        stick_cal_x_l[0] = stick_cal_x_l[1] - (((factory_stick_cal[7] << 8) & 0xF00) | factory_stick_cal[6]);
        stick_cal_y_l[0] = stick_cal_y_l[1] - ((factory_stick_cal[8] << 4) | (factory_stick_cal[7] >> 4));
        stick_cal_x_l[2] = stick_cal_x_l[1] + (((factory_stick_cal[1] << 8) & 0xF00) | factory_stick_cal[0]);
        stick_cal_y_l[2] = stick_cal_y_l[1] + ((factory_stick_cal[2] << 4) | (factory_stick_cal[2] >> 4));
    }
    else {
        // no calibration
    }
    if (_joycon_type != 1) {
        stick_cal_x_r[1] = ((factory_stick_cal[10] << 8) & 0xF00) | factory_stick_cal[9];
        stick_cal_y_r[1] = (factory_stick_cal[11] << 4) | (factory_stick_cal[10] >> 4);
        stick_cal_x_r[0] = stick_cal_x_r[1] - (((factory_stick_cal[13] << 8) & 0xF00) | factory_stick_cal[12]);
        stick_cal_y_r[0] = stick_cal_y_r[1] - ((factory_stick_cal[14] << 4) | (factory_stick_cal[13] >> 4));
        stick_cal_x_r[2] = stick_cal_x_r[1] + (((factory_stick_cal[16] << 8) & 0xF00) | factory_stick_cal[15]);
        stick_cal_y_r[2] = stick_cal_y_r[1] + ((factory_stick_cal[17] << 4) | (factory_stick_cal[16] >> 4));
    }
    else {
        // no calibration
    }

    if ((user_stick_cal[0] | user_stick_cal[1] << 8) == 0xA1B2) {
        stick_cal_x_l[1] = ((user_stick_cal[6] << 8) & 0xF00) | user_stick_cal[5];
        stick_cal_y_l[1] = (user_stick_cal[7] << 4) | (user_stick_cal[6] >> 4);
        stick_cal_x_l[0] = stick_cal_x_l[1] - (((user_stick_cal[9] << 8) & 0xF00) | user_stick_cal[8]);
        stick_cal_y_l[0] = stick_cal_y_l[1] - ((user_stick_cal[10] << 4) | (user_stick_cal[9] >> 4));
        stick_cal_x_l[2] = stick_cal_x_l[1] + (((user_stick_cal[3] << 8) & 0xF00) | user_stick_cal[2]);
        stick_cal_y_l[2] = stick_cal_y_l[1] + ((user_stick_cal[4] << 4) | (user_stick_cal[3] >> 4));
    }
    else {
        // no calibration
    }
    if ((user_stick_cal[0xB] | user_stick_cal[0xC] << 8) == 0xA1B2) {
        stick_cal_x_r[1] = ((user_stick_cal[14] << 8) & 0xF00) | user_stick_cal[13];
        stick_cal_y_r[1] = (user_stick_cal[15] << 4) | (user_stick_cal[14] >> 4);
        stick_cal_x_r[0] = stick_cal_x_r[1] - (((user_stick_cal[17] << 8) & 0xF00) | user_stick_cal[16]);
        stick_cal_y_r[0] = stick_cal_y_r[1] - ((user_stick_cal[18] << 4) | (user_stick_cal[17] >> 4));
        stick_cal_x_r[2] = stick_cal_x_r[1] + (((user_stick_cal[20] << 8) & 0xF00) | user_stick_cal[19]);
        stick_cal_y_r[2] = stick_cal_y_r[1] + ((user_stick_cal[21] << 4) | (user_stick_cal[20] >> 4));
    }
    else {
        // no calibration
    }

    sensor_cal[0][0] = static_cast<int16_t>(factory_sensor_cal[0] | factory_sensor_cal[1] << 8);
    sensor_cal[0][1] = static_cast<int16_t>(factory_sensor_cal[2] | factory_sensor_cal[3] << 8);
    sensor_cal[0][2] = static_cast<int16_t>(factory_sensor_cal[4] | factory_sensor_cal[5] << 8);

    // Gyro cal origin position
    sensor_cal[1][0] = static_cast<int16_t>(factory_sensor_cal[0xC] | factory_sensor_cal[0xD] << 8);
    sensor_cal[1][1] = static_cast<int16_t>(factory_sensor_cal[0xE] | factory_sensor_cal[0xF] << 8);
    sensor_cal[1][2] = static_cast<int16_t>(factory_sensor_cal[0x10] | factory_sensor_cal[0x11] << 8);

    if ((user_sensor_cal[0x0] | user_sensor_cal[0x1] << 8) == 0xA1B2) {
        // Acc cal origin position
        sensor_cal[0][0] = static_cast<int16_t>(user_sensor_cal[2]) | static_cast<int16_t>(user_sensor_cal[3] << 8);
        sensor_cal[0][1] = static_cast<int16_t>(user_sensor_cal[4]) | static_cast<int16_t>(user_sensor_cal[5] << 8);
        sensor_cal[0][2] = static_cast<int16_t>(user_sensor_cal[6]) | static_cast<int16_t>(user_sensor_cal[7] << 8);

        // Gyro cal origin position
        sensor_cal[1][0] = static_cast<int16_t>(user_sensor_cal[0xE]) | static_cast<int16_t>(user_sensor_cal[0xF] << 8);
        sensor_cal[1][1] = static_cast<int16_t>(user_sensor_cal[0x10]) | static_cast<int16_t>(user_sensor_cal[0x11] << 8);
        sensor_cal[1][2] = static_cast<int16_t>(user_sensor_cal[0x12]) | static_cast<int16_t>(user_sensor_cal[0x13] << 8);
    }
    else {
        // no calibration
    }

    // Enable nxpad standard input report
    uint8_t buf[49];
    int res;
    res = writePacket(buf, 49, RUMBLE_PLUS, SET_INPUT_MODE, 0x30);
    res = hid_read_timeout(handle, buf, sizeof(buf), 120);
    res = writePacket(buf, 49, RUMBLE_PLUS, ENABLE_IMU, 0x01);
    res = hid_read_timeout(handle, buf, sizeof(buf), 120);


    // Use SPI calibration and convert them to SI acc unit (m/s^2)
    //acc_cal_coeff[0] = (float)(1.0 / (float)(16384 - uint16_to_int16(sensor_cal[0][0]))) * 4.0f  * 9.8f;
    acc_cal_coeff[0] = (float)(1.0 / (float)(16384 - sensor_cal[0][0])) * 4.0f  * 9.8f;
    acc_cal_coeff[1] = (float)(1.0 / (float)(16384 - sensor_cal[0][1])) * 4.0f  * 9.8f;
    acc_cal_coeff[2] = (float)(1.0 / (float)(16384 - sensor_cal[0][2])) * 4.0f  * 9.8f;

    // Use SPI calibration and convert them to SI gyro unit (rad/s)
    gyro_cal_coeff[0] = (float)(936.0 / (float)(13371 - sensor_cal[1][0]) * 0.01745329251994);
    gyro_cal_coeff[1] = (float)(936.0 / (float)(13371 - sensor_cal[1][1]) * 0.01745329251994);
    gyro_cal_coeff[2] = (float)(936.0 / (float)(13371 - sensor_cal[1][2]) * 0.01745329251994);


    emit deviceStatusMessage("Input streaming started...");
    onInputPollTimerTimeout();
    return;
}


/////////////////////////
//// IR Camera Code ////
/////////////////////////
void JoyConWorker::onIrConfigUpdated(ir_image_config ir_new_config) {
    qDebug()<<"got camera config";
    QString error_msg;
    _ir_config = ir_new_config;

    // Configure the IR camera and take a photo or stream.
    int res = ir_sensor(_ir_config);

    // Get error
    switch (res) {
    case 1:
        error_msg = "1ID31";
        break;
    case 2:
        error_msg = "2MCUON";
        break;
    case 3:
        error_msg = "3MCUONBUSY";
        break;
    case 4:
        error_msg = "4MCUMODESET";
        break;
    case 5:
        error_msg = "5MCUSETBUSY";
        break;
    case 6:
        error_msg = "6IRMODESET";
        break;
    case 7:
        error_msg = "7IRSETBUSY";
        break;
    case 8:
        error_msg = "8IRCFG";
        break;
    case 9:
        error_msg = "9IRFCFG";
        break;
    default:
        break;
    }
    if (res > 0) {
        emit deviceStatusMessage("IR camera status: Error " + error_msg + "!");
    }
    //return res;
    requestJoyConInputs();
}

int JoyConWorker::ir_sensor_auto_exposure(int white_pixels_percent) {
    int res;
    uint8_t buf[49];
    uint16_t new_exposure = 0;
    int old_exposure = 0;//(uint16_t)FormJoy::myform1->numeric_IRExposure->Value;

    // Calculate new exposure;
    if (white_pixels_percent == 0)
        old_exposure += 10;
    else if (white_pixels_percent > 5)
        old_exposure -= (white_pixels_percent / 4) * 20;

    old_exposure = CLAMP(old_exposure, 0, 600);
    //FormJoy::myform1->numeric_IRExposure->Value = old_exposure;
    new_exposure = old_exposure * 31200 / 1000;

    memset(buf, 0, sizeof(buf));
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = 0x01;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    pkt->subcmd = 0x21;

    pkt->subcmd_21_23_04.mcu_cmd    = 0x23; // Write register cmd
    pkt->subcmd_21_23_04.mcu_subcmd = 0x04; // Write register to IR mode subcmd
    pkt->subcmd_21_23_04.no_of_reg  = 0x03; // Number of registers to write. Max 9.

    pkt->subcmd_21_23_04.reg1_addr = 0x3001; // R: 0x0130 - Set Exposure time LSByte
    pkt->subcmd_21_23_04.reg1_val  = new_exposure & 0xFF;
    pkt->subcmd_21_23_04.reg2_addr = 0x3101; // R: 0x0131 - Set Exposure time MSByte
    pkt->subcmd_21_23_04.reg2_val  = (new_exposure & 0xFF00) >> 8;
    pkt->subcmd_21_23_04.reg3_addr = 0x0700; // R: 0x0007 - Finalize config - Without this, the register changes do not have any effect.
    pkt->subcmd_21_23_04.reg3_val  = 0x01;

    buf[48] = mcu_crc8_calc(buf + 12, 36);
    res = hid_write(handle, buf, sizeof(buf));

    return res;
}


int JoyConWorker::get_raw_ir_image(unsigned char show_status) {
    QString ir_status;

    qint64 elapsed_time = 0;
    qint64 elapsed_time2 = 0;
    qint64 start_time = QDateTime::currentMSecsSinceEpoch();
    //System::Diagnostics::Stopwatch^ sw = System::Diagnostics::Stopwatch::StartNew();

    uint8_t buf[49];
    uint8_t buf_reply[0x170];
    uint8_t *buf_image = new uint8_t[19 * 4096]; // 8bpp greyscale image.
    uint16_t bad_signal = 0;
    int error_reading = 0;
    float noise_level = 0.0f;
    int avg_intensity_percent = 0.0f;
    int previous_frag_no = 0;
    int got_frag_no = 0;
    int missed_packet_no = 0;
    bool missed_packet = false;
    int initialization = 2;
    int max_pixels = ((_ir_config.max_fragment_no < 218 ? _ir_config.max_fragment_no : 217) + 1) * 300;
    int white_pixels_percent = 0;

    memset(buf_image, 0, sizeof(buf_image));

    memset(buf, 0, sizeof(buf));
    memset(buf_reply, 0, sizeof(buf_reply));
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = 0x11;
    pkt->subcmd = 0x03;
    buf[48] = 0xFF;

    // First ack
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    buf[14] = 0x0;
    buf[47] = mcu_crc8_calc(buf + 11, 36);
    hid_write(handle, buf, sizeof(buf));


    while (enable_IRVideoPhoto || initialization) {
        memset(buf_reply, 0, sizeof(buf_reply));
        hid_read_timeout(handle, buf_reply, sizeof(buf_reply), 200);

        //Check if new packet
        if (buf_reply[0] == 0x31 && buf_reply[49] == 0x03) {
            got_frag_no = buf_reply[52];
            if (got_frag_no == (previous_frag_no + 1) % (_ir_config.max_fragment_no + 1)) {

                previous_frag_no = got_frag_no;

                // ACK for fragment
                hdr->timer = _timing_byte & 0xF;
                _timing_byte++;
                buf[14] = previous_frag_no;
                buf[47] = mcu_crc8_calc(buf + 11, 36);
                hid_write(handle, buf, sizeof(buf));

                memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                // Auto exposure.
                // TODO: Fix placement, so it doesn't drop next fragment.
                // if (enable_IRAutoExposure && initialization < 2 && got_frag_no == 0){
                //     white_pixels_percent = (int)((*(u16*)&buf_reply[55] * 100) / max_pixels);
                //     ir_sensor_auto_exposure(white_pixels_percent);
                // }

                // Status percentage
                ir_status.clear();
                if (initialization < 2) {
                    if (show_status == 2)
                        emit deviceStatusMessage("IR: Streaming...");
                    else
                        emit deviceStatusMessage("IR: Receiving..." );
                }
                else
                    emit deviceStatusMessage("Initializing IR...");

                //debug
                // printf("%02X Frag: Copy\n", got_frag_no);

                //FormJoy::myform1->lbl_IRStatus->Text = gcnew String(ir_status.str().c_str()) + (sw->ElapsedMilliseconds - elapsed_time).ToString() + "ms";
                elapsed_time = QDateTime::currentMSecsSinceEpoch() - start_time;

                // Check if final fragment. Draw the frame.
                if (got_frag_no == _ir_config.max_fragment_no) {
                    QImage img(buf_image, _ir_config.width, _ir_config.height, QImage::Format_Indexed8);

                    emit newCameraImage(img);
                    //debug
                    //printf("%02X Frag: Draw -------\n", got_frag_no);

                    // Stats/IR header parsing
                    // buf_reply[53]: Average Intensity. 0-255 scale.
                    // buf_reply[54]: Unknown. Shows up only when EXFilter is enabled.
                    // *(u16*)&buf_reply[55]: White pixels (pixels with 255 value). Max 65535. Uint16 constraints, even though max is 76800.
                    // *(u16*)&buf_reply[57]: Pixels with ambient noise from external light sources (sun, lighter, IR remotes, etc). Cleaned by External Light Filter.
                    noise_level = (float)(*(uint16_t*)&buf_reply[57]) / ((float)(*(uint16_t*)&buf_reply[55]) + 1.0f);
                    white_pixels_percent = (int)((*(uint16_t*)&buf_reply[55] * 100) / max_pixels);
                    avg_intensity_percent = (int)((buf_reply[53] * 100) / 255);
                    // FormJoy::myform1->lbl_IRHelp->Text = String::Format("Amb Noise: {0:f2},  Int: {1:D}%,  FPS: {2:D} ({3:D}ms)\nEXFilter: {4:D},  White Px: {5:D}%,  EXF Int: {6:D}",
                    //     noise_level, avg_intensity_percent, (int)(1000 / elapsed_time2), elapsed_time2, *(u16*)&buf_reply[57], white_pixels_percent, buf_reply[54]);

                    //elapsed_time2 = sw->ElapsedMilliseconds;

                    if (initialization)
                        initialization--;
                }
                //Application::DoEvents();
            }
            // Repeat/Missed fragment
            else if (got_frag_no  || previous_frag_no) {
                // Check if repeat ACK should be send. Avoid writing to image buffer.
                if (got_frag_no == previous_frag_no) {
                    //debug
                    //printf("%02X Frag: Repeat\n", got_frag_no);

                    // ACK for fragment
                    hdr->timer = _timing_byte & 0xF;
                    _timing_byte++;
                    buf[14] = got_frag_no;
                    buf[47] = mcu_crc8_calc(buf + 11, 36);
                    hid_write(handle, buf, sizeof(buf));

                    missed_packet = false;
                }
                // Check if missed fragment and request it.
                else if(missed_packet_no != got_frag_no && !missed_packet) {
                    if (_ir_config.max_fragment_no != 0x03) {
                        //debug
                        //printf("%02X Frag: Missed %02X, Prev: %02X, PrevM: %02X\n", got_frag_no, previous_frag_no + 1, previous_frag_no, missed_packet_no);

                        // Missed packet
                        hdr->timer = _timing_byte & 0xF;
                        _timing_byte++;
                        //Request for missed packet. You send what the next fragment number will be, instead of the actual missed packet.
                        buf[12] = 0x1;
                        buf[13] = previous_frag_no + 1;
                        buf[14] = 0;
                        buf[47] = mcu_crc8_calc(buf + 11, 36);
                        hid_write(handle, buf, sizeof(buf));

                        buf[12] = 0x00;
                        buf[13] = 0x00;

                        memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                        previous_frag_no = got_frag_no;
                        missed_packet_no = got_frag_no - 1;
                        missed_packet = true;
                    }
                    // Check if missed fragment and res is 30x40. Don't request it.
                    else {
                        //debug
                        //printf("%02X Frag: Missed but res is 30x40\n", got_frag_no);

                        // ACK for fragment
                        hdr->timer = _timing_byte & 0xF;
                        _timing_byte++;
                        buf[14] = got_frag_no;
                        buf[47] = mcu_crc8_calc(buf + 11, 36);
                        hid_write(handle, buf, sizeof(buf));

                        memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                        previous_frag_no = got_frag_no;
                    }
                }
                // Got the requested missed fragments.
                else if (missed_packet_no == got_frag_no){
                    //debug
                    //printf("%02X Frag: Got missed %02X\n", got_frag_no, missed_packet_no);

                    // ACK for fragment
                    hdr->timer = _timing_byte & 0xF;
                    _timing_byte++;
                    buf[14] = got_frag_no;
                    buf[47] = mcu_crc8_calc(buf + 11, 36);
                    hid_write(handle, buf, sizeof(buf));

                    memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                    previous_frag_no = got_frag_no;
                    missed_packet = false;
                }
                // Repeat of fragment that is not max fragment.
                else {
                    //debug
                    //printf("%02X Frag: RepeatWoot M:%02X\n", got_frag_no, missed_packet_no);

                    // ACK for fragment
                    hdr->timer = _timing_byte & 0xF;
                    _timing_byte++;
                    buf[14] = got_frag_no;
                    buf[47] = mcu_crc8_calc(buf + 11, 36);
                    hid_write(handle, buf, sizeof(buf));
                }

                // Status percentage
                ir_status.clear();
                if (initialization < 2) {
                    //if (show_status == 2)
                    //    ir_status = "Status: Streaming.. ";
                    //else
                    //    ir_status = "Status: Receiving.. ";
                }
                else
                    ir_status = "Status: Initializing.. ";
                // ir_status << std::setfill(' ') << std::setw(3);
                // ir_status << std::fixed << std::setprecision(0) << (float)got_frag_no / (float)(ir_max_frag_no + 1) * 100.0f;
                // ir_status << "% - ";

                // FormJoy::myform1->lbl_IRStatus->Text = gcnew String(ir_status.str().c_str()) + (sw->ElapsedMilliseconds - elapsed_time).ToString() + "ms";
                //elapsed_time = sw->ElapsedMilliseconds;
                //Application::DoEvents();
            }

            // Streaming start
            else {
                // ACK for fragment
                hdr->timer = _timing_byte & 0xF;
                _timing_byte++;
                buf[14] = got_frag_no;
                buf[47] = mcu_crc8_calc(buf + 11, 36);
                hid_write(handle, buf, sizeof(buf));

                memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                //debug
                //printf("%02X Frag: 0 %02X\n", buf_reply[52], previous_frag_no);

                //FormJoy::myform1->lbl_IRStatus->Text = (sw->ElapsedMilliseconds - elapsed_time).ToString() + "ms";
                //elapsed_time = sw->ElapsedMilliseconds;
                //Application::DoEvents();

                previous_frag_no = 0;
            }

        }
        // Empty IR report. Send Ack again. Otherwise, it fallbacks to high latency mode (30ms per data fragment)
        else if (buf_reply[0] == 0x31) {
            // ACK for fragment
            hdr->timer = _timing_byte & 0xF;
            _timing_byte++;

            // Send ACK again or request missed frag
            if (buf_reply[49] == 0xFF) {
                buf[14] = previous_frag_no;
            }
            else if (buf_reply[49] == 0x00) {
                buf[12] = 0x1;
                buf[13] = previous_frag_no + 1;
                buf[14] = 0;
                // printf("%02X Mode: Missed next packet %02X\n", buf_reply[49], previous_frag_no + 1);
            }

            buf[47] = mcu_crc8_calc(buf + 11, 36);
            hid_write(handle, buf, sizeof(buf));

            buf[12] = 0x00;
            buf[13] = 0x00;
        }
    }

    delete[] buf_image;

    /*
    if(_irCaptureTimer == nullptr) {
        _irCaptureTimer = new QTimer(this);
        _irCaptureTimer->setTimerType(Qt::PreciseTimer);
        connect(_irCaptureTimer, SIGNAL(timeout()),
                this, SLOT(onIrCaptureTimerTimeout()));
        _irCaptureTimer->start();
    }
    else {
        _irCaptureTimer->start();
    }*/

    qDebug()<<"capture completed";
    return 0;
}

// non-blocking polling currently not working
void JoyConWorker::onIrCaptureTimerTimeout()
{
    qDebug()<<"capture timer timeout";
    QString ir_status;

    qint64 elapsed_time = 0;
    qint64 elapsed_time2 = 0;
    qint64 start_time = QDateTime::currentMSecsSinceEpoch();
    //System::Diagnostics::Stopwatch^ sw = System::Diagnostics::Stopwatch::StartNew();

    uint8_t buf[49];
    uint8_t buf_reply[0x170];
    uint8_t *buf_image = new uint8_t[19 * 4096]; // 8bpp greyscale image.
    uint16_t bad_signal = 0;
    int error_reading = 0;
    float noise_level = 0.0f;
    int avg_intensity_percent = 0.0f;
    int previous_frag_no = 0;
    int got_frag_no = 0;
    int missed_packet_no = 0;
    bool missed_packet = false;
    int initialization = 2;
    int max_pixels = ((_ir_config.max_fragment_no < 218 ? _ir_config.max_fragment_no : 217) + 1) * 300;
    int white_pixels_percent = 0;

    memset(buf_image, 0, sizeof(buf_image));

    memset(buf, 0, sizeof(buf));
    memset(buf_reply, 0, sizeof(buf_reply));

    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);

    /*
    hdr->cmd = 0x11;
    pkt->subcmd = 0x03;
    buf[48] = 0xFF;

    // First ack
    hdr->timer = timming_byte & 0xF;
    timming_byte++;
    buf[14] = 0x0;
    buf[47] = mcu_crc8_calc(buf + 11, 36);
    hid_write(handle, buf, sizeof(buf));
    */

    // IR Read/ACK loop for fragmented data packets.
    // It also avoids requesting missed data fragments, we just skip it to not complicate things.
    //while (enable_IRVideoPhoto || initialization) {
    memset(buf_reply, 0, sizeof(buf_reply));
    hid_read_timeout(handle, buf_reply, sizeof(buf_reply), 200);

    //Check if new packet
    if (buf_reply[0] == 0x31 && buf_reply[49] == 0x03) {
        got_frag_no = buf_reply[52];
        if (got_frag_no == (previous_frag_no + 1) % (_ir_config.max_fragment_no + 1)) {

            previous_frag_no = got_frag_no;

            // ACK for fragment
            hdr->timer = _timing_byte & 0xF;
            _timing_byte++;
            buf[14] = previous_frag_no;
            buf[47] = mcu_crc8_calc(buf + 11, 36);
            hid_write(handle, buf, sizeof(buf));

            memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

            // Auto exposure.
            // TODO: Fix placement, so it doesn't drop next fragment.
            // if (enable_IRAutoExposure && initialization < 2 && got_frag_no == 0){
            //     white_pixels_percent = (int)((*(u16*)&buf_reply[55] * 100) / max_pixels);
            //     ir_sensor_auto_exposure(white_pixels_percent);
            // }

            // Status percentage
            ir_status.clear();
            if (initialization < 2) {
                // if (show_status == 2)
                //     ir_status = "Status: Streaming.. ";
                //else
                //   ir_status = "Status: Receiving.. ";
            }
            else
                ir_status = "Status: Initializing.. ";
            // ir_status << std::setfill(' ') << std::setw(3);
            // ir_status << std::fixed << std::setprecision(0) << (float)got_frag_no / (float)(ir_max_frag_no + 1) * 100.0f;
            // ir_status << "% - ";

            //debug
            // printf("%02X Frag: Copy\n", got_frag_no);

            //FormJoy::myform1->lbl_IRStatus->Text = gcnew String(ir_status.str().c_str()) + (sw->ElapsedMilliseconds - elapsed_time).ToString() + "ms";
            elapsed_time = QDateTime::currentMSecsSinceEpoch() - start_time;

            // Check if final fragment. Draw the frame.
            if (got_frag_no == _ir_config.max_fragment_no) {
                // Update Viewport
                //elapsed_time2 = sw->ElapsedMilliseconds - elapsed_time2;
                //qDebug()<<"got frame";
                //QImage img;
                QImage img(buf_image, _ir_config.width, _ir_config.height, QImage::Format_Indexed8);
                //img.loadFromData(buf_image, 320*240);
                //const char format = 'XBM';
                //bool ok = img.loadFromData(buf_image, 19 * 4096, &format);
                //qDebug()<<"conversion ok?"<<ok;

                emit newCameraImage(img);
                /*QList<unsigned char> img_list;
                    img_list.reserve(19 * 4096);
                    for(int i = 0; i < 19 * 4096; ++i){
                        img_list.append(buf_image[i]);
                    }
                    emit newCameraImage(img_list);*/
                //FormJoy::myform1->setIRPictureWindow(buf_image, true);

                //debug
                //printf("%02X Frag: Draw -------\n", got_frag_no);

                // Stats/IR header parsing
                // buf_reply[53]: Average Intensity. 0-255 scale.
                // buf_reply[54]: Unknown. Shows up only when EXFilter is enabled.
                // *(u16*)&buf_reply[55]: White pixels (pixels with 255 value). Max 65535. Uint16 constraints, even though max is 76800.
                // *(u16*)&buf_reply[57]: Pixels with ambient noise from external light sources (sun, lighter, IR remotes, etc). Cleaned by External Light Filter.
                noise_level = (float)(*(uint16_t*)&buf_reply[57]) / ((float)(*(uint16_t*)&buf_reply[55]) + 1.0f);
                white_pixels_percent = (int)((*(uint16_t*)&buf_reply[55] * 100) / max_pixels);
                avg_intensity_percent = (int)((buf_reply[53] * 100) / 255);
                // FormJoy::myform1->lbl_IRHelp->Text = String::Format("Amb Noise: {0:f2},  Int: {1:D}%,  FPS: {2:D} ({3:D}ms)\nEXFilter: {4:D},  White Px: {5:D}%,  EXF Int: {6:D}",
                //     noise_level, avg_intensity_percent, (int)(1000 / elapsed_time2), elapsed_time2, *(u16*)&buf_reply[57], white_pixels_percent, buf_reply[54]);

                //elapsed_time2 = sw->ElapsedMilliseconds;

                if (initialization)
                    initialization--;
            }
            //Application::DoEvents();
        }
        // Repeat/Missed fragment
        else if (got_frag_no  || previous_frag_no) {
            // Check if repeat ACK should be send. Avoid writing to image buffer.
            if (got_frag_no == previous_frag_no) {
                //debug
                //printf("%02X Frag: Repeat\n", got_frag_no);

                // ACK for fragment
                hdr->timer = _timing_byte & 0xF;
                _timing_byte++;
                buf[14] = got_frag_no;
                buf[47] = mcu_crc8_calc(buf + 11, 36);
                hid_write(handle, buf, sizeof(buf));

                missed_packet = false;
            }
            // Check if missed fragment and request it.
            else if(missed_packet_no != got_frag_no && !missed_packet) {
                if (_ir_config.max_fragment_no != 0x03) {
                    //debug
                    //printf("%02X Frag: Missed %02X, Prev: %02X, PrevM: %02X\n", got_frag_no, previous_frag_no + 1, previous_frag_no, missed_packet_no);

                    // Missed packet
                    hdr->timer = _timing_byte & 0xF;
                    _timing_byte++;
                    //Request for missed packet. You send what the next fragment number will be, instead of the actual missed packet.
                    buf[12] = 0x1;
                    buf[13] = previous_frag_no + 1;
                    buf[14] = 0;
                    buf[47] = mcu_crc8_calc(buf + 11, 36);
                    hid_write(handle, buf, sizeof(buf));

                    buf[12] = 0x00;
                    buf[13] = 0x00;

                    memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                    previous_frag_no = got_frag_no;
                    missed_packet_no = got_frag_no - 1;
                    missed_packet = true;
                }
                // Check if missed fragment and res is 30x40. Don't request it.
                else {
                    //debug
                    //printf("%02X Frag: Missed but res is 30x40\n", got_frag_no);

                    // ACK for fragment
                    hdr->timer = _timing_byte & 0xF;
                    _timing_byte++;
                    buf[14] = got_frag_no;
                    buf[47] = mcu_crc8_calc(buf + 11, 36);
                    hid_write(handle, buf, sizeof(buf));

                    memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                    previous_frag_no = got_frag_no;
                }
            }
            // Got the requested missed fragments.
            else if (missed_packet_no == got_frag_no){
                //debug
                //printf("%02X Frag: Got missed %02X\n", got_frag_no, missed_packet_no);

                // ACK for fragment
                hdr->timer = _timing_byte & 0xF;
                _timing_byte++;
                buf[14] = got_frag_no;
                buf[47] = mcu_crc8_calc(buf + 11, 36);
                hid_write(handle, buf, sizeof(buf));

                memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

                previous_frag_no = got_frag_no;
                missed_packet = false;
            }
            // Repeat of fragment that is not max fragment.
            else {
                //debug
                //printf("%02X Frag: RepeatWoot M:%02X\n", got_frag_no, missed_packet_no);

                // ACK for fragment
                hdr->timer = _timing_byte & 0xF;
                _timing_byte++;
                buf[14] = got_frag_no;
                buf[47] = mcu_crc8_calc(buf + 11, 36);
                hid_write(handle, buf, sizeof(buf));
            }

            // Status percentage
            ir_status.clear();
            if (initialization < 2) {
                //if (show_status == 2)
                //    ir_status = "Status: Streaming.. ";
                //else
                //    ir_status = "Status: Receiving.. ";
            }
            else
                ir_status = "Status: Initializing.. ";
            // ir_status << std::setfill(' ') << std::setw(3);
            // ir_status << std::fixed << std::setprecision(0) << (float)got_frag_no / (float)(ir_max_frag_no + 1) * 100.0f;
            // ir_status << "% - ";

            // FormJoy::myform1->lbl_IRStatus->Text = gcnew String(ir_status.str().c_str()) + (sw->ElapsedMilliseconds - elapsed_time).ToString() + "ms";
            //elapsed_time = sw->ElapsedMilliseconds;
            //Application::DoEvents();
        }

        // Streaming start
        else {
            // ACK for fragment
            hdr->timer = _timing_byte & 0xF;
            _timing_byte++;
            buf[14] = got_frag_no;
            buf[47] = mcu_crc8_calc(buf + 11, 36);
            hid_write(handle, buf, sizeof(buf));

            memcpy(buf_image + (300 * got_frag_no), buf_reply + 59, 300);

            //debug
            //printf("%02X Frag: 0 %02X\n", buf_reply[52], previous_frag_no);

            //FormJoy::myform1->lbl_IRStatus->Text = (sw->ElapsedMilliseconds - elapsed_time).ToString() + "ms";
            //elapsed_time = sw->ElapsedMilliseconds;
            //Application::DoEvents();

            previous_frag_no = 0;
        }

    }
    // Empty IR report. Send Ack again. Otherwise, it fallbacks to high latency mode (30ms per data fragment)
    else if (buf_reply[0] == 0x31) {
        // ACK for fragment
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;

        // Send ACK again or request missed frag
        if (buf_reply[49] == 0xFF) {
            buf[14] = previous_frag_no;
        }
        else if (buf_reply[49] == 0x00) {
            buf[12] = 0x1;
            buf[13] = previous_frag_no + 1;
            buf[14] = 0;
            // printf("%02X Mode: Missed next packet %02X\n", buf_reply[49], previous_frag_no + 1);
        }

        buf[47] = mcu_crc8_calc(buf + 11, 36);
        hid_write(handle, buf, sizeof(buf));

        buf[12] = 0x00;
        buf[13] = 0x00;
    }
    //}

    delete[] buf_image;
}


int JoyConWorker::ir_sensor(ir_image_config &ir_cfg)
{
    int res;
    uint8_t buf[0x170];
    static int output_buffer_length = 49;
    int error_reading = 0;
    int res_get = 0;
    // Set input report to x31
    while (1) {
        writePacket(buf, sizeof(buf), RUMBLE_PLUS, SET_INPUT_MODE, 0x31);
        //res = hid_write(handle, buf, output_buffer_length); //TK output buffer length was 49

        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x0380)
                goto step1;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 1;
            goto step10;
        }
    }

step1:
    // Enable MCU
    error_reading = 0;
    while (1) {
        writePacket(buf, sizeof(buf), RUMBLE_PLUS, STM32_SET_STATE, 0x1);
        //res = hid_write(handle, buf, output_buffer_length);

        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x2280)
                goto step2;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 2;
            goto step10;
        }
    }

step2:
    // Request MCU mode status
    error_reading = 0;
    while (1) { // Not necessary, but we keep to make sure the MCU is ready.
        writePacket(buf, sizeof(buf), MCU_DATA, BT_PAIR);
        //res = hid_write(handle, buf, output_buffer_length);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x31) {
                //if (buf[49] == 0x01 && buf[56] == 0x06) // MCU state is Initializing
                // *(u16*)buf[52]LE x04 in lower than 3.89fw, x05 in 3.89
                // *(u16*)buf[54]LE x12 in lower than 3.89fw, x18 in 3.89
                // buf[56]: mcu mode state
                if (buf[49] == 0x01 && buf[56] == 0x01) // MCU state is Standby
                    goto step3;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 3;
            goto step10;
        }
    }

step3:
    // Set MCU mode
    error_reading = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = RUMBLE_PLUS;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = STM32_CONFIG;

        pkt->subcmd_21_21.mcu_cmd    = 0x21; // Set MCU mode cmd
        pkt->subcmd_21_21.mcu_subcmd = 0x00; // Set MCU mode cmd
        pkt->subcmd_21_21.mcu_mode   = 0x05; // MCU mode - 1: Standby, 4: NFC, 5: IR, 6: Initializing/FW Update?

        buf[48] = mcu_crc8_calc(buf + 12, 36);
        res = hid_write(handle, buf, output_buffer_length);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x21) {
                // *(u16*)buf[18]LE x04 in lower than 3.89fw, x05 in 3.89
                // *(u16*)buf[20]LE x12 in lower than 3.89fw, x18 in 3.89
                // buf[56]: mcu mode state
                if (buf[15] == 0x01 && *(uint32_t*)&buf[22] == 0x01) // Mcu mode is Standby
                    goto step4;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 4;
            goto step10;
        }
    }

step4:
    // Request MCU mode status
    error_reading = 0;
    while (1) { // Not necessary, but we keep to make sure the MCU mode changed.
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = MCU_DATA;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = BT_PAIR;
        res = hid_write(handle, buf, output_buffer_length);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x31) {
                // *(u16*)buf[52]LE x04 in lower than 3.89fw, x05 in 3.89
                // *(u16*)buf[54]LE x12 in lower than 3.89fw, x18 in 3.89
                if (buf[49] == 0x01 && buf[56] == 0x05) // Mcu mode is IR
                    goto step5;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 5;
            goto step10;
        }
    }

step5:
    // Set IR mode and number of packets for each data blob. Blob size is packets * 300 bytes.
    error_reading = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = RUMBLE_PLUS;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;

        pkt->subcmd = STM32_CONFIG;
        pkt->subcmd_21_23_01.mcu_cmd     = 0x23;
        pkt->subcmd_21_23_01.mcu_subcmd  = 0x01; // Set IR mode cmd
        pkt->subcmd_21_23_01.mcu_ir_mode = 0x07; // IR mode - 2: No mode/Disable?, 3: Moment, 4: Dpd (Wii-style pointing), 6: Clustering,
        // 7: Image transfer, 8-10: Hand analysis (Silhouette, Image, Silhouette/Image), 0,1/5/10+: Unknown
        pkt->subcmd_21_23_01.no_of_frags = _ir_config.max_fragment_no; // Set number of packets to output per buffer
        pkt->subcmd_21_23_01.mcu_major_v = 0x0500; // Set required IR MCU FW v5.18. Major 0x0005.
        pkt->subcmd_21_23_01.mcu_minor_v = 0x1800; // Set required IR MCU FW v5.18. Minor 0x0018.

        buf[48] = mcu_crc8_calc(buf + 12, 36);
        res = hid_write(handle, buf, output_buffer_length);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x21) {
                // Mode set Ack
                if (buf[15] == 0x0b)
                    goto step6;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 6;
            goto step10;
        }
    }

step6:
    // Request IR mode status
    error_reading = 0;
    while (0) { // Not necessary
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 0x11;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;

        pkt->subcmd = 0x03;
        pkt->subcmd_arg.arg1 = 0x02;

        buf[47] = mcu_crc8_calc(buf + 11, 36);
        buf[48] = 0xFF;
        res = hid_write(handle, buf, output_buffer_length);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x31) {
                // mode set to 7: Image transfer
                if (buf[49] == 0x13 && *(uint16_t*)&buf[50] == 0x0700)
                    goto step7;
            }
            retries++;
            if (retries > 4 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 7;
            goto step10;
        }
    }

step7:
    // Write to registers for the selected IR mode
    error_reading = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 0x01;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x21;

        pkt->subcmd_21_23_04.mcu_cmd    = 0x23; // Write register cmd
        pkt->subcmd_21_23_04.mcu_subcmd = 0x04; // Write register to IR mode subcmd
        pkt->subcmd_21_23_04.no_of_reg  = 0x09; // Number of registers to write. Max 9.

        pkt->subcmd_21_23_04.reg1_addr  = 0x2e00; // R: 0x002e - Set Resolution based on sensor binning and skipping
        pkt->subcmd_21_23_04.reg1_val   = ir_cfg.ir_res_reg;
        pkt->subcmd_21_23_04.reg2_addr  = 0x3001; // R: 0x0130 - Set Exposure time LSByte - (31200 * us /1000) & 0xFF - Max: 600us, Max encoded: 0x4920.
        pkt->subcmd_21_23_04.reg2_val   = ir_cfg.ir_exposure & 0xFF;
        pkt->subcmd_21_23_04.reg3_addr  = 0x3101; // R: 0x0131 - Set Exposure time MSByte - ((31200 * us /1000) & 0xFF00) >> 8
        pkt->subcmd_21_23_04.reg3_val   = (ir_cfg.ir_exposure & 0xFF00) >> 8;
        pkt->subcmd_21_23_04.reg4_addr  = 0x3201; // R: 0x0132 - Enable Max exposure Time - 0: Manual exposure, 1: Max exposure
        pkt->subcmd_21_23_04.reg4_val   = 0x00;
        pkt->subcmd_21_23_04.reg5_addr  = 0x1000; // R: 0x0010 - Set IR Leds groups state - Only 3 LSB usable
        pkt->subcmd_21_23_04.reg5_val   = ir_cfg.ir_leds;
        pkt->subcmd_21_23_04.reg6_addr  = 0x2e01; // R: 0x012e - Set digital gain LSB 4 bits of the value - 0-0xff
        pkt->subcmd_21_23_04.reg6_val   = (ir_cfg.ir_digital_gain & 0xF) << 4;
        pkt->subcmd_21_23_04.reg7_addr  = 0x2f01; // R: 0x012f - Set digital gain MSB 4 bits of the value - 0-0x7
        pkt->subcmd_21_23_04.reg7_val   = (ir_cfg.ir_digital_gain & 0xF0) >> 4;
        pkt->subcmd_21_23_04.reg8_addr  = 0x0e00; // R: 0x00e0 - External light filter - LS o bit0: Off/On, bit1: 0x/1x, bit2: ??, bit4,5: ??.
        pkt->subcmd_21_23_04.reg8_val   = ir_cfg.ir_ex_light_filter;
        pkt->subcmd_21_23_04.reg9_addr  = 0x4301; // R: 0x0143 - ExLF/White pixel stats threshold - 200: Default
        pkt->subcmd_21_23_04.reg9_val   = 0xc8;

        buf[48] = mcu_crc8_calc(buf + 12, 36);
        res = hid_write(handle, buf, output_buffer_length);

        // Request IR mode status, before waiting for the x21 ack
        memset(buf, 0, sizeof(buf));
        hdr->cmd = 0x11;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x03;
        pkt->subcmd_arg.arg1 = 0x02;
        buf[47] = mcu_crc8_calc(buf + 11, 36);
        buf[48] = 0xFF;
        res = hid_write(handle, buf, output_buffer_length);

        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x21) {
                // Registers for mode 7: Image transfer set
                if (buf[15] == 0x13 && *(uint16_t*)&buf[16] == 0x0700)
                    goto step8;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 8;
            goto step10;
        }
    }

step8:
    // Write to registers for the selected IR mode
    error_reading = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 0x01;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x21;

        pkt->subcmd_21_23_04.mcu_cmd    = 0x23; // Write register cmd
        pkt->subcmd_21_23_04.mcu_subcmd = 0x04; // Write register to IR mode subcmd
        pkt->subcmd_21_23_04.no_of_reg  = 0x08; // Number of registers to write. Max 9.

        pkt->subcmd_21_23_04.reg1_addr  = 0x1100; // R: 0x0011 - Leds 1/2 Intensity - Max 0x0F.
        pkt->subcmd_21_23_04.reg1_val   = (ir_cfg.ir_leds_intensity >> 8) & 0xFF;
        pkt->subcmd_21_23_04.reg2_addr  = 0x1200; // R: 0x0012 - Leds 3/4 Intensity - Max 0x10.
        pkt->subcmd_21_23_04.reg2_val   = ir_cfg.ir_leds_intensity & 0xFF;
        pkt->subcmd_21_23_04.reg3_addr  = 0x2d00; // R: 0x002d - Flip image - 0: Normal, 1: Vertically, 2: Horizontally, 3: Both
        pkt->subcmd_21_23_04.reg3_val   = ir_cfg.ir_flip;
        pkt->subcmd_21_23_04.reg4_addr  = 0x6701; // R: 0x0167 - Enable De-noise smoothing algorithms - 0: Disable, 1: Enable.
        pkt->subcmd_21_23_04.reg4_val   = (ir_cfg.ir_denoise >> 16) & 0xFF;
        pkt->subcmd_21_23_04.reg5_addr  = 0x6801; // R: 0x0168 - Edge smoothing threshold - Max 0xFF, Default 0x23
        pkt->subcmd_21_23_04.reg5_val   = (ir_cfg.ir_denoise >> 8) & 0xFF;
        pkt->subcmd_21_23_04.reg6_addr  = 0x6901; // R: 0x0169 - Color Interpolation threshold - Max 0xFF, Default 0x44
        pkt->subcmd_21_23_04.reg6_val   = ir_cfg.ir_denoise & 0xFF;
        pkt->subcmd_21_23_04.reg7_addr  = 0x0400; // R: 0x0004 - LSB Buffer Update Time - Default 0x32
        if (ir_cfg.ir_res_reg == 0x69)
            pkt->subcmd_21_23_04.reg7_val = 0x2d; // A value of <= 0x2d is fast enough for 30 x 40, so the first fragment has the updated frame.
        else
            pkt->subcmd_21_23_04.reg7_val = 0x32; // All the other resolutions the default is enough. Otherwise a lower value can break hand analysis.
        pkt->subcmd_21_23_04.reg8_addr  = 0x0700; // R: 0x0007 - Finalize config - Without this, the register changes do not have any effect.
        pkt->subcmd_21_23_04.reg8_val   = 0x01;

        buf[48] = mcu_crc8_calc(buf + 12, 36);
        res = hid_write(handle, buf, output_buffer_length);

        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[0] == 0x21) {
                // Registers for mode 7: Image transfer set
                if (buf[15] == 0x13 && *(uint16_t*)&buf[16] == 0x0700)
                    goto step9;
                // If the Joy-Con gets to reply to the previous x11 - x03 02 cmd before sending the above,
                // it will reply with the following if we do not send x11 - x03 02 again:
                else if (buf[15] == 0x23) // Got mcu mode config write.
                    goto step9;
            }
            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            res_get = 9;
            goto step10;
        }
    }

step9:
    // Stream or Capture images from NIR Camera
    if (enable_IRVideoPhoto)
        res_get = get_raw_ir_image(2);// stream video
    else
        res_get = get_raw_ir_image(1);//capoture a photo

    //////
    // TODO: Should we send subcmd x21 with 'x230102' to disable IR mode before disabling MCU?
step10:

    /*8 uint8_t buf[49];
    int res = writePacket(buf, 49, RUMBLE_PLUS, SET_INPUT_MODE, 0x3F);
    res = hid_read_timeout(handle, buf, 0, 64);*/

    // Disable MCU
    memset(buf, 0, sizeof(buf));
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = 1;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    pkt->subcmd = 0x22;
    pkt->subcmd_arg.arg1 = 0x00;
    res = hid_write(handle, buf, output_buffer_length);
    res = hid_read_timeout(handle, buf, sizeof(buf), 64);


    // Set input report back to x3f
    error_reading = 0;
    while (1) {
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        hdr->cmd = 1;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x03;
        pkt->subcmd_arg.arg1 = 0x3f;
        res = hid_write(handle, buf, output_buffer_length);
        int retries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (*(uint16_t*)&buf[0xD] == 0x0380)
                goto stepf;

            retries++;
            if (retries > 8 || res == 0)
                break;
        }
        error_reading++;
        if (error_reading > 7) {
            goto stepf;
        }
    }

stepf:
    return res_get;
}


int JoyConWorker::get_ir_registers(int start_reg, int reg_group) {
    int res;
    uint8_t buf[0x170];
    static int output_buffer_length = 49;
    int error_reading = 0;
    int res_get = 0;

    // Get the IR registers
    error_reading = 0;
    int pos_ir_registers = start_reg;
    while (1) {
repeat_send:
        memset(buf, 0, sizeof(buf));
        auto hdr = (brcm_hdr *)buf;
        auto pkt = (brcm_cmd_01 *)(hdr + 1);
        memset(buf, 0, sizeof(buf));
        hdr->cmd = 0x11;
        hdr->timer = _timing_byte & 0xF;
        _timing_byte++;
        pkt->subcmd = 0x03;
        pkt->subcmd_arg.arg1 = 0x03;

        buf[12] = 0x1; // seems to be always 0x01

        buf[13] = pos_ir_registers; // 0-4 registers page/group
        buf[14] = 0x00; // offset. this plus the number of registers, must be less than x7f
        buf[15] = 0x7f; // Number of registers to show + 1

        buf[47] = mcu_crc8_calc(buf + 11, 36);

        res = hid_write(handle, buf, output_buffer_length);

        int tries = 0;
        while (1) {
            res = hid_read_timeout(handle, buf, sizeof(buf), 64);
            if (buf[49] == 0x1b && buf[51] == pos_ir_registers && buf[52] == 0x00) {
                error_reading = 0;

                // print out some info
                QString result;
                QTextStream(&result) << ("--->%02X, %02X : %02X:\n", buf[51], buf[52], buf[53]);

                /* QString str("--->%02X, %02X : %02X:\n")
                        .arg( buf[51])
                        .arg( buf[52])
                        .arg( buf[53]);*/
                //printf("--->%02X, %02X : %02X:\n", buf[51], buf[52], buf[53]);

                for (int i = 0; i <= buf[52] + buf[53]; i++)
                    if ((i & 0xF) == 0xF)
                        printf("%02X | ", buf[54 + i]);
                    else
                        printf("%02X ", buf[54 + i]);
                printf("\n");
                break;
            }
            tries++;
            if (tries > 8) {
                error_reading++;
                if (error_reading > 5) {
                    return 1;
                }
                goto repeat_send;
            }

        }
        pos_ir_registers++;
        if (pos_ir_registers > reg_group) {
            break;
        }

    }
    printf("\n");

    qDebug()<<"...camera config done";
    return 0;
}


int JoyConWorker::ir_sensor_config_live(ir_image_config &ir_cfg) {
    int res;
    uint8_t buf[49];

    memset(buf, 0, sizeof(buf));
    auto hdr = (brcm_hdr *)buf;
    auto pkt = (brcm_cmd_01 *)(hdr + 1);
    hdr->cmd = 0x01;
    hdr->timer = _timing_byte & 0xF;
    _timing_byte++;
    pkt->subcmd = 0x21;

    pkt->subcmd_21_23_04.mcu_cmd    = 0x23; // Write register cmd
    pkt->subcmd_21_23_04.mcu_subcmd = 0x04; // Write register to IR mode subcmd
    pkt->subcmd_21_23_04.no_of_reg  = 0x09; // Number of registers to write. Max 9.

    pkt->subcmd_21_23_04.reg1_addr = 0x3001; // R: 0x0130 - Set Exposure time LSByte
    pkt->subcmd_21_23_04.reg1_val  = ir_cfg.ir_exposure & 0xFF;
    pkt->subcmd_21_23_04.reg2_addr = 0x3101; // R: 0x0131 - Set Exposure time MSByte
    pkt->subcmd_21_23_04.reg2_val  = (ir_cfg.ir_exposure & 0xFF00) >> 8;
    pkt->subcmd_21_23_04.reg3_addr = 0x1000; // R: 0x0010 - Set IR Leds groups state
    pkt->subcmd_21_23_04.reg3_val  = ir_cfg.ir_leds;
    pkt->subcmd_21_23_04.reg4_addr = 0x2e01; // R: 0x012e - Set digital gain LSB 4 bits
    pkt->subcmd_21_23_04.reg4_val  = (ir_cfg.ir_digital_gain & 0xF) << 4;
    pkt->subcmd_21_23_04.reg5_addr = 0x2f01; // R: 0x012f - Set digital gain MSB 4 bits
    pkt->subcmd_21_23_04.reg5_val  = (ir_cfg.ir_digital_gain & 0xF0) >> 4;
    pkt->subcmd_21_23_04.reg6_addr = 0x0e00; // R: 0x00e0 - External light filter
    pkt->subcmd_21_23_04.reg6_val  = ir_cfg.ir_ex_light_filter;
    pkt->subcmd_21_23_04.reg7_addr = (ir_cfg.ir_custom_register & 0xFF) << 8 | (ir_cfg.ir_custom_register >> 8) & 0xFF;
    pkt->subcmd_21_23_04.reg7_val  = (ir_cfg.ir_custom_register >> 16) & 0xFF;
    pkt->subcmd_21_23_04.reg8_addr = 0x1100; // R: 0x0011 - Leds 1/2 Intensity - Max 0x0F.
    pkt->subcmd_21_23_04.reg8_val  = (ir_cfg.ir_leds_intensity >> 8) & 0xFF;
    pkt->subcmd_21_23_04.reg9_addr = 0x1200; // R: 0x0012 - Leds 3/4 Intensity - Max 0x10.
    pkt->subcmd_21_23_04.reg9_val  = ir_cfg.ir_leds_intensity & 0xFF;

    buf[48] = mcu_crc8_calc(buf + 12, 36);
    res = hid_write(handle, buf, sizeof(buf));

    // sleep to prevent a dropped packet
    this->thread()->msleep(15);

    pkt->subcmd_21_23_04.no_of_reg = 0x06; // Number of registers to write. Max 9.

    pkt->subcmd_21_23_04.reg1_addr = 0x2d00; // R: 0x002d - Flip image - 0: Normal, 1: Vertically, 2: Horizontally, 3: Both
    pkt->subcmd_21_23_04.reg1_val  = ir_cfg.ir_flip;
    pkt->subcmd_21_23_04.reg2_addr = 0x6701; // R: 0x0167 - Enable De-noise smoothing algorithms - 0: Disable, 1: Enable.
    pkt->subcmd_21_23_04.reg2_val  = (ir_cfg.ir_denoise >> 16) & 0xFF;
    pkt->subcmd_21_23_04.reg3_addr = 0x6801; // R: 0x0168 - Edge smoothing threshold - Max 0xFF, Default 0x23
    pkt->subcmd_21_23_04.reg3_val  = (ir_cfg.ir_denoise >> 8) & 0xFF;
    pkt->subcmd_21_23_04.reg4_addr = 0x6901; // R: 0x0169 - Color Interpolation threshold - Max 0xFF, Default 0x44
    pkt->subcmd_21_23_04.reg4_val  = ir_cfg.ir_denoise & 0xFF;
    pkt->subcmd_21_23_04.reg5_addr = 0x0400; // R: 0x0004 - LSB Buffer Update Time - Default 0x32
    if (ir_cfg.ir_res_reg == 0x69)
        pkt->subcmd_21_23_04.reg5_val = 0x2d; // A value of <= 0x2d is fast enough for 30 x 40, so the first fragment has the updated frame.
    else
        pkt->subcmd_21_23_04.reg5_val = 0x32; // All the other resolutions the default is enough. Otherwise a lower value can break hand analysis.
    pkt->subcmd_21_23_04.reg6_addr = 0x0700; // R: 0x0007 - Finalize config - Without this, the register changes do not have any effect.
    pkt->subcmd_21_23_04.reg6_val  = 0x01;

    buf[48] = mcu_crc8_calc(buf + 12, 36);
    res = hid_write(handle, buf, sizeof(buf));

    // get_ir_registers(0,4); // Get all register pages
    // get_ir_registers((ir_cfg.ir_custom_register >> 8) & 0xFF, (ir_cfg.ir_custom_register >> 8) & 0xFF); // Get all registers based on changed register's page

    return res;
}

int JoyConWorker::send_custom_command(uint8_t* arg)
{
    int res_write;
    int res = 0;
    uint8_t buf_cmd[49];
    uint8_t buf_reply[0x170];
    memset(buf_cmd, 0, sizeof(buf_cmd));
    memset(buf_reply, 0, sizeof(buf_reply));

    buf_cmd[0] = arg[0]; // cmd
    buf_cmd[1] = _timing_byte & 0xF;
    _timing_byte++;
    // Vibration pattern
    buf_cmd[2] = buf_cmd[6] = arg[1];
    buf_cmd[3] = buf_cmd[7] = arg[2];
    buf_cmd[4] = buf_cmd[8] = arg[3];
    buf_cmd[5] = buf_cmd[9] = arg[4];

    buf_cmd[10] = arg[5]; // subcmd

    // subcmd x21 crc byte
    if (arg[5] == 0x21)
        arg[43] = mcu_crc8_calc(arg + 7, 36);

    if (buf_cmd[0] == 0x01 || buf_cmd[0] == 0x10 || buf_cmd[0] == 0x11) {
        for (int i = 6; i < 44; i++) {
            buf_cmd[5 + i] = arg[i];
        }
    }
    //Use subcmd after command
    else {
        for (int i = 6; i < 44; i++) {
            buf_cmd[i - 5] = arg[i];
        }
    }

    //Packet size header + subcommand and uint8 argument
    res_write = hid_write(handle, buf_cmd, sizeof(buf_cmd));

    int retries = 0;
    while (1) {
        res = hid_read_timeout(handle, buf_reply, sizeof(buf_reply), 64);

        if (res > 0) {
            if (arg[0] == 0x01 && buf_reply[0] == 0x21)
                break;
            else if (arg[0] != 0x01)
                break;
        }

        retries++;
        if (retries == 20)
            break;
    }
    if (res > 12) {
        if (buf_reply[0] == 0x21 || buf_reply[0] == 0x30 || buf_reply[0] == 0x33 || buf_reply[0] == 0x31 || buf_reply[0] == 0x3F) {
            qDebug()<<"reply 1";

        }
        else {
            qDebug()<<"reply 2";
        }
    }
    else if (res > 0 && res <= 12) {
        qDebug()<<"reply 3";
    }
    else {
        qDebug()<<"no reply";
    }

    return 0;
}
