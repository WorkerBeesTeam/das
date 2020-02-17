#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>

#include <QModbusRtuSerialSlave>
#include <QTcpSocket>

#include <vector>

//#include "simplemodbusdevice.h"
#include "device_table_item.h"
#include <plus/das/database.h>

#include "Das/scheme.h"
#include "Das/type_managers.h"

class QProcess;
class QSettings;
class QTreeView;

void term_handler(int);

struct Config
{
    Config() :
        baud_rate_(QSerialPort::Baud115200),
        data_bits_(QSerialPort::Data8),
        parity_(QSerialPort::NoParity),
        stop_bits_(QSerialPort::OneStop),
        flow_control_(QSerialPort::NoFlowControl)
    {
    }

    QString name_;
    int baud_rate_;                           ///< Скорость. По умолчанию 115200
    QSerialPort::DataBits   data_bits_;       ///< Количество бит. По умолчанию 8
    QSerialPort::Parity     parity_;         ///< Паритет. По умолчанию нет
    QSerialPort::StopBits   stop_bits_;       ///< Стоп бит. По умолчанию 1
    QSerialPort::FlowControl flow_control_;   ///< Управление потоком. По умолчанию нет
};

namespace Ui
{
class Main_Window;
}

class DevicesTableModel;
class Main_Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Main_Window(QWidget *parent = 0);
    ~Main_Window();

    Q_INVOKABLE QString ttyPath();
private slots:
    void handleDeviceError(QModbusDevice::Error newError);
    void onStateChanged(int state);

    void changeTemperature();
    void proccessData();
    void socketDataReady();
    void on_openBtn_toggled(bool open);
    void on_socatReset_clicked();
    void on_favorites_only_toggled(bool checked);
    void on_tree_view_customContextMenuRequested(const QPoint &pos);

private:

    struct Socat_Info
    {
//        SocatInfo() : socat(nullptr) {}
//        ~SocatInfo() { delete socat; }
        QProcess* socat_process_;
        QString port_from_name_;
        QString port_to_name_;
    };    

    struct Modbus_Device_Item : Socat_Info
    {
        QModbusRtuSerialSlave* modbus_device_;
        QSerialPort* serial_port_;
        DeviceTableItem* device_table_item_;

        void operator =(const Socat_Info& info)
        {
            socat_process_ = info.socat_process_;
            port_from_name_ = info.port_from_name_;
            port_to_name_ = info.port_to_name_;
        }
    };

    Ui::Main_Window *ui_;

    Das::DB::Helper db_manager_;
    Das::Scheme das_scheme_;

    QTimer temp_timer_;

    QSerialPort* serial_port_;
    QProcess* socat_process_;

    std::map<uchar, Modbus_Device_Item> modbus_list_;

    Config config_;

    void init();
    void init_database();
    void fill_data();

    QString get_user();

    Socat_Info create_socat();
    void init_client_connection();

    void expand_tree_view();

    DevicesTableModel* devices_table_model_;

    QString tty_path_to_;
    friend void term_handler(int);
};

#endif // MAINWINDOW_H
