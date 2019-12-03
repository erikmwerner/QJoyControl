#ifndef JOYCONTHREAD_H
#define JOYCONTHREAD_H

#include <QObject>
#include <QTimer>
//#include <cstdint>
#include "hidapi.h"
#include <QImage>

//struct hid_device;
//#include <QMetaType>

class EventHandler;

Q_DECLARE_METATYPE(QList<double>)
Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<unsigned char>)

const unsigned short NINTENDO = 1406; // 0x057e
const unsigned short JOYCON_L = 8198; // 0x2006
const unsigned short JOYCON_R = 8199; // 0x2007
const unsigned short PRO_CONTROLLER = 8201; // 0x2009


// Limit a value to the range [low, high]
template <typename T> T CLAMP(const T& value, const T& low, const T& high)
{
    return value < low ? low : (value > high ? high : value);
}

// From: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/bluetooth_hid_notes.md
enum JC_CMD {
    // outputs:
    RUMBLE_PLUS = 0x01, // Output. Send a rumble and subcommand
    MCU_FW_UPDATE = 0x03, // Output. Send a FW update packet to the STM32 (IR/NFC MCU)
    RUMBLE_ONLY = 0x10, // Output. Send rumble data only
    MCU_DATA = 0x11, // Output. Request data from the STM32. Can also send rumble
    //0x12 // Unknown. Does the same as 0x28

    // inputs
    PUSH_INPUT = 0x3F,
    STANDARD_INPUT = 0x21, // Standard input reports used for subcommand replies
    MCU_UPDATE = 0x23, // NFC/IR MCU FW update input report.
    FULL_INPUT = 0x30, // Standard full mode - input reports with IMU data instead of subcommand replies. Pushes current state @60Hz, or @120Hz if Pro Controller.
    LARGE_PACKET = 0x31, // NFC/IR MCU mode. Pushes large packets with standard input report + NFC/IR MCU data input report.
    //0x32, // Unknown. Sends standard input reports.
    //0x33, // Unknown. Sends standard input reports.

    // feature reports
    GET_LAST_SUB = 0x02, // Get last subcommand reply
    ENABLE_OTA_FW = 0x70, // Enable OTA FW upgrade
    SETUP_MEM_READ = 0x71, // Setup memory read
    MEM_READ = 0x72, // Memory read
    MEM_ERASE = 0x73, // Memory sector erase
    MEM_WRITE = 0x74, // Memory sector write
    REBOOT = 0x75 // Reboots and executes the firmware rom in the given address
    // 0xCC, // Unknown send feature report
    // 0xFE, // Unknown get feature report
};

enum JC_SUBCMD {
    GET_STATE = 0x00,
    BT_PAIR = 0x01,
    GET_INFO = 0x02,
    SET_INPUT_MODE = 0x03,
    TRIGGER_ELAPSED = 0x04,
    GET_PAGE_LIST_STATE = 0x05,
    SET_HCI_STATE = 0x06,
    RESET_PAIR = 0x07,
    LOW_POWER = 0x08,
    SPI_READ = 0x10,
    SPI_WRITE = 0x11,
    SPI_ERASE = 0x12,
    STM32_RESET = 0x20,
    STM32_CONFIG = 0x21,
    STM32_SET_STATE = 0x22,
    // 0x24,
    // 0x25,
    // 0x28,
    // 0x29
    SET_GPIO2_VALUE = 0x2A,
    GET_STM32_Data = 0x2B,
    SET_PLAYER_LED = 0x30,
    GET_PLAYER_LED = 0x31,
    SET_HOME_LED = 0x38,
    ENABLE_IMU = 0x40,
    CONFIG_IMU = 0x41,
    WRITE_IMU = 0x42,
    READ_IMU = 0x43,
    ENABLE_VIB = 0x48,
    GET_VREG = 0x50,
    SET_GPIO1_VALUE = 0x51,
    GET_GPIO_VALUE = 0x52
};

#pragma pack(push, 1) // set current alignment packing value to 1 (MSVC)

// a structure to hold a
struct brcm_hdr {           // 10 bytes total
    uint8_t cmd;            // 0
    uint8_t timer;          // 1
    uint8_t rumble_l[4];    // 2-5
    uint8_t rumble_r[4];    // 6-9
};

struct brcm_cmd_01 { // 49 bytes long
    uint8_t subcmd;
    union {
        struct {
            uint32_t offset;
            uint8_t size;
        } spi_data;

        struct {
            uint8_t arg1;
            uint8_t arg2;
        } subcmd_arg;

        struct {
            uint8_t  mcu_cmd;
            uint8_t  mcu_subcmd;
            uint8_t  mcu_mode;
        } subcmd_21_21;

        struct {
            uint8_t  mcu_cmd;
            uint8_t  mcu_subcmd;
            uint8_t  no_of_reg;
            uint16_t reg1_addr;
            uint8_t  reg1_val;
            uint16_t reg2_addr;
            uint8_t  reg2_val;
            uint16_t reg3_addr;
            uint8_t  reg3_val;
            uint16_t reg4_addr;
            uint8_t  reg4_val;
            uint16_t reg5_addr;
            uint8_t  reg5_val;
            uint16_t reg6_addr;
            uint8_t  reg6_val;
            uint16_t reg7_addr;
            uint8_t  reg7_val;
            uint16_t reg8_addr;
            uint8_t  reg8_val;
            uint16_t reg9_addr;
            uint8_t  reg9_val;
        } subcmd_21_23_04;

        struct {
            uint8_t  mcu_cmd;
            uint8_t  mcu_subcmd;
            uint8_t  mcu_ir_mode;
            uint8_t  no_of_frags;
            uint16_t mcu_major_v;
            uint16_t mcu_minor_v;
        } subcmd_21_23_01;
    };
};

struct ir_image_config {
    uint8_t  ir_res_reg;
    uint16_t ir_exposure;
    uint8_t  ir_leds; // Leds to enable, Strobe/Flashlight modes
    uint16_t ir_leds_intensity; // MSByte: Leds 1/2, LSB: Leds 3/4
    uint8_t  ir_digital_gain;
    uint8_t  ir_ex_light_filter;
    uint32_t ir_custom_register; // MSByte: Enable/Disable, Middle Byte: Edge smoothing, LSB: Color interpolation
    uint16_t ir_buffer_update_time;
    uint8_t  ir_hand_analysis_mode;
    uint8_t  ir_hand_analysis_threshold;
    uint32_t ir_denoise; // MSByte: Enable/Disable, Middle Byte: Edge smoothing, LSB: Color interpolation
    uint8_t  ir_flip;
    int  width;
    int  height;
    uint8_t  max_fragment_no;// = 0xff;
};
Q_DECLARE_METATYPE(ir_image_config)

#pragma pack(pop) // remove the last pack pragma

class JoyConWorker : public QObject
{
    Q_OBJECT
public:
    explicit JoyConWorker(QObject *parent = nullptr);
    ~JoyConWorker();

signals:

    //< returns an empty QString and zero if nothing is connected
    void deviceConnectionChanged(QString, const unsigned short);

    void deviceStatusMessage(QString); //< send messages to display on the status bar

    void newDeviceInfo(QList<unsigned char>, int);
    void newTemperatureData(float);
    void newBatteryData(int, int, int);
    void newCameraImage(QImage);

    //< digital input data, analog input data
    void newInputData(QList<int>, QList<double>);

    void finished();

public slots:
    void setup();
    void onConnectHID(unsigned short vendor_id,
                       unsigned short product_id,
                       const wchar_t *serial_number);
    void onDisconnectHID();

    void requestDeviceInfo();
    void requestTemperature();
    void requestBattery();
    void requestJoyConInputs();

    void onInputStreamingEnabled(bool enabled);

    void onIrConfigUpdated(ir_image_config ir_new_config);
    int get_raw_ir_image(unsigned char show_status = 0); //tk show status?

private slots:
    void onInputPollTimerTimeout();
    void onDeviceStatusTimerTimeout();
    void onIrCaptureTimerTimeout();

private:
    QTimer* _input_poll_timer = nullptr; //<check for new input data to process
    QTimer* _device_status_timer = nullptr; //<check for new input data to process
    QTimer *_irCaptureTimer = nullptr;
    bool _stream_buttons = false;
    int _joycon_type = -1; //<
    hid_device* handle = nullptr;

    // This is a global packet number that needs to be incremented by 1 for each packet sent
    // It loops in 0x0 - 0xF range. (Decimal 0-15)
    uint8_t _timing_byte = 0x0;

    int set_led_busy();

    int get_spi_data(uint32_t offset, const uint16_t read_len, uint8_t *test_buf);
    int write_spi_data(uint32_t offset, const uint16_t write_len, uint8_t *test_buf);

    int get_device_info(uint8_t *test_buf);
    int get_battery(uint8_t *test_buf);
    int get_temperature(uint8_t *test_buf);
    std::string get_sn(uint32_t offset, const uint16_t read_len);
    int send_rumble();

    int ir_sensor(ir_image_config &ir_cfg);
    int ir_sensor_config_live(ir_image_config &ir_cfg);
    int get_ir_registers(int start_reg, int reg_group);

    bool enable_IRVideoPhoto = false; //< true = stream images (video)

    int ir_sensor_auto_exposure(int white_pixels_percent);
    int writePacket(uint8_t *buf, unsigned long len, uint8_t cmd, uint8_t subcmd = 0, uint8_t arg1 = 0, uint8_t arg2 = 0);

    float acc_cal_coeff[3]; // x,y,z
    float gyro_cal_coeff[3]; // x,y,z

    uint8_t factory_stick_cal[0x12]; // 18 bytes
    uint8_t user_stick_cal[0x16]; // 22 bytes
    uint8_t sensor_model[0x6];
    uint8_t stick_model[0x24];
    uint8_t factory_sensor_cal[0x18];
    uint8_t user_sensor_cal[0x1A];
    int16_t sensor_cal[2][3];
    uint16_t stick_cal_x_l[3];
    uint16_t stick_cal_y_l[3];
    uint16_t stick_cal_x_r[3];
    uint16_t stick_cal_y_r[3];

    ir_image_config _ir_config;
    int send_custom_command(uint8_t *arg);
};


#endif // JOYCONTHREAD_H
