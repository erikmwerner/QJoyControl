#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

#include "joyconworker.h"

// https://stackoverflow.com/questions/40599038/on-linux-whats-a-good-way-to-to-use-hid-reports-over-usb

class QThread;
class StatusWidget;
class InputMapWidget;

namespace Ui {
class MainWindow;
}

/*!
 * \brief The MainWindow class is the root of the QJoyControl GUI.
 * It can be shown or hidden, but it will remain alive until the application
 * is closed.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /*!
     * \brief MainWindow default constructor
     * \param parent usually a nullptr
     */
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void onShow();
    void onHide();

    void onCameraImageData(QImage img);
    void hideAndClose();

    void onNewInputData(QList<int> button_data, QList<double> analog_data);
    void onDeviceConnectionChanged(QString sn, const unsigned short pid);
    void onJoyConStatusMessage(QString message);

signals:
    void connectHID(unsigned short vendor_id,
                          unsigned short product_id,
                          const wchar_t *serial_number);

    void disconnectHID();

    void enableInputStreaming(bool);

    void updateIrConfig(ir_image_config);
    void captureImage(unsigned char);

private slots:
    void onDeviceInfoData(QList<unsigned char> device_info, int joycon_type);
    void onTemperatureData(float temp_c);
    void onBatteryData(int volt, int percent, int status);

    void on_pushButtonCapture_clicked();
    void on_pushButtonStream_clicked();

    void on_checkBoxDiagnostics_toggled(bool checked);
    void on_checkBoxAppNap_toggled(bool checked);
    void on_checkBoxIdleSleep_toggled(bool checked);

    void on_toolButtonRefresh_clicked();
    void on_toolButtonConnect_clicked();
    void on_toolButtonDisconnect_clicked();

    void on_checkBoxInvertIcon_toggled(bool checked);

    void on_pushButtonSaveImage_clicked();

    void on_comboBoxColorMap_currentIndexChanged(int index);

    void showAbout();

    void on_checkBoxLeftClick_toggled(bool checked);

    void on_checkBoxRightClick_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    StatusWidget* _status_widget;
    EventHandler* _event_handler = nullptr;
    QSystemTrayIcon *_tray_icon = nullptr;
    QMenu *_tray_icon_menu;
    JoyConWorker* _worker;
    QThread* _thread;

    QAction *_hide_action;
    QAction *_show_action;
    QAction *_about_action;
    QAction *_quit_action;

    bool _force_close = false;

    unsigned short _joycon_pid = 0;
    QString _joycon_sn = QString();
    // hold the last button state received
    int _last_button_state[3] = {0,0,0};
    QImage _current_image = QImage();
    QVector<QRgb> _color_table;
    InputMapWidget* _l_mapper = nullptr;
    InputMapWidget* _zl_mapper = nullptr;
    InputMapWidget* _r_mapper = nullptr;
    InputMapWidget* _zr_mapper = nullptr;

    qint64 _time_last_update; // keep track of the last time the GUI was updated


    void loadSettings();
    void setupThread();
    void addHidItem(struct hid_device_info* info);
    void sendCameraConfig();
    void saveSettings();
    void handleButtons(QList<int> buttons);
    double scaleAnalog(double input);
    void createTrayIcon(bool is_inverted);
    void testButton(int last_button_state, int _last_button_state, int mask);
};

#endif // MAINWINDOW_H
