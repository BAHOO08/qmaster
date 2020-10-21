//------------------------------------------------------------------------------
//
//
//
//
//
//------------------------------------------------------------------------------
/*!
 * \file
 * \brief Main window
 * \copyright maisvendoo
 * \author Dmitry Pritykin
 * \date
 */

#include    "mainwindow.h"
#include    "ui_mainwindow.h"
#include <QDebug>
#include    <QtSerialPort/QSerialPortInfo>
#include    <QString>
#include    <QHeaderView>
#include    <QSpinBox>
#include    <QTableView>
#include    <QComboBox>
#include    <QPushButton>
#include    <QPlainTextEdit>
#include    <QTimer>
#include    <QCheckBox>
#include    <QStyleFactory>
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
enum
{
    TAB_DATA_TYPE = 0,
    TAB_ADDRESS = 1,
    TAB_DATA = 2
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
enum
{
    UPDATE_PORT_LIST_TIMEOUT = 100
};

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette p = qApp->palette();
    p.setColor(QPalette::Window, QColor(53,53,53));
    p.setColor(QPalette::Button, QColor(53,53,53));
    p.setColor(QPalette::Highlight, QColor(142,45,197));
    p.setColor(QPalette::ButtonText, QColor(255,255,255));
    p.setColor(QPalette::WindowText, QColor(255,255,255));
    qApp->setPalette(p);


    init();   
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::init()
{
    // Set window icon
    QIcon icon(":/icons/img/logo.png");
    setWindowIcon(icon);

    // Setup timer, for update ports list
    portsListUpdateTimer = new QTimer(this);

    connect(portsListUpdateTimer, &QTimer::timeout,
            this, &MainWindow::updatePortsList);

    currentSerialPorts = 0;
    portsListUpdateTimer->start(UPDATE_PORT_LIST_TIMEOUT);


    // Modbus functuions list to UI
    mb_func.insert("Read Coils (0x01)", MB_FUNC_READ_COILS);
    mb_func.insert("Read Discrete Inputs (0x02)", MB_FUNC_READ_DISCRETE_INPUT);
    mb_func.insert("Read Holding Registers (0x03)", MB_FUNC_READ_HOLDING_REGISTERS);
    mb_func.insert("Read Input Registers (0x04)", MB_FUNC_READ_INPUT_REGISTERS);
    mb_func.insert("Write Coil (0x05)", MB_FUNC_WRITE_COIL);
    mb_func.insert("Write Holding Register (0x06)", MB_FUNC_WRITE_HOLDING_REGISTER);
    mb_func.insert("Write Multiple Coils (0x0F)", MB_FUNC_WRITE_MULTIPLE_COILS);
    mb_func.insert("Write Multiple Registers (0x10)", MB_FUNC_WRITE_MULTIPLE_REGISTERS);

    QMap<QString, int>::iterator it;

    for (it = mb_func.begin(); it != mb_func.end(); ++it)
    {
        ui->cbFunc->addItem(it.key());
    }

    // Tune output data table
    QTableWidget *table = ui->tableData;
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    addTableRow(table);
    table->horizontalHeader()->setVisible(true);

    connect(ui->sbCount, SIGNAL(valueChanged(int)),
            this, SLOT(changeDataTableRowsCount(int)));

    connect(ui->sbAddress, SIGNAL(valueChanged(int)),
            this, SLOT(changeAddress(int)));

    connect(ui->cbFunc, &QComboBox::currentTextChanged,
            this, &MainWindow::changedFunc);

    QString dataType = getDataTypeName(mb_func[ui->cbFunc->currentText()]);

    ui->tableData->setItem(0, TAB_DATA_TYPE, new QTableWidgetItem(dataType));

    // Modbus master initialization
    master = new Master();

    connect(this, &MainWindow::sendMasterRequest,
            master, &Master::sendRequest);

    connect(ui->bConnect, &QPushButton::released,
            this, &MainWindow::onConnectRelease);

    connect(master, &Master::statusPrint,
            this, &MainWindow::statusPrint);

    connect(master, &Master::statusPrint,
            this, &MainWindow::printMsg);

    // Send button initialize
    connect(ui->bSend, &QPushButton::released, this, &MainWindow::sendButtonRelease);

    qRegisterMetaType<answer_request_t>();

    connect(master, &Master::sendAnswer, this, &MainWindow::onSlaveAnswer);

    connect(master, &Master::sendRawData, this, &MainWindow::onRawDataReceive);

    connect(ui->bRawDataClean, &QPushButton::released,
            this, &MainWindow::onRawDataClean);

    // Prepare work of data sender
    is_send_started = false;
    is_cyclic = false;

    connect(ui->cCyclicSend, &QCheckBox::stateChanged,
            this, &MainWindow::checkCyclicSend);

    connect(&dataSender, &DataSender::sendMasterRequest,
            master, &Master::sendRequest);

    connect(&threadCyclicSend, &QThread::finished,
            this, &MainWindow::onFinishSendThread);

    connect(&dataSender, &DataSender::quit,
            &threadCyclicSend, &QThread::terminate);

    connect(&dataSender, &DataSender::isStarted,
            this, &MainWindow::getStartedFlag);


    //Кнопки сбор/разбор
    //connect(ui->build_schemes, &QPushButton::released, this, &MainWindow::buildButtonRelease);
  //  connect(ui->destroy_schemes, &QPushButton::released,this, &MainWindow::destroyButtonRelease);
    // Setup timer, for update ports list
    updateDats = new QTimer(this);

    connect(updateDats, &QTimer::timeout,
            this, &MainWindow::updateData);
   // connect(&dataSender,&DataSender::index_inc,this,&MainWindow::index_inc_realise);
    updateDats->start(100);
    is_close_event = false;
    index = 0;

    ///Прячем ненужное
    ui->cbParity->hide();
    ui->label_5->hide();

    ui->cbStopBits->hide();
    ui->label_4->hide();


    ui->cbDataBits->hide();
    ui->label_3->hide();

    ui->cbBaud->hide();
    ui->label_2->hide();

    
    ui->bSend->hide();

    ui->sbCount->hide();
    ui->slaveID_4->hide();

    ui->sbAddress->hide();
    ui->slaveID_3->hide();

    
    ui->cbFunc->hide();
    ui->slaveID_2->hide();

    ui->sbSlaveID->hide();
    ui->lSlaveID->hide();

    
    ui->label_7->hide();
    ui->cCyclicSend->hide();
    ui->sbSendInterval->hide();
    ui->slaveID_5->hide();
    ui->tableData->hide();
    ui->ptRawData->hide();
    ui->bRawDataClean->hide();
    ui->label_6->hide();
    //sizePolicy().Maximum
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
serial_config_t MainWindow::getSerialConfig()
{
    serial_config_t s_cfg;

    s_cfg.portName = ui->cbPort->currentText();
    s_cfg.baudrate = ui->cbBaud->currentText().toInt();
    s_cfg.dataBits = ui->cbDataBits->currentText().toInt();
    s_cfg.stopBits = ui->cbStopBits->currentText().toInt();
    s_cfg.parity = ui->cbParity->currentText();

    return s_cfg;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::addTableRow(QTableWidget *table)
{
    int idx = table->rowCount();
    table->insertRow(table->rowCount());

    int addr = ui->sbAddress->value() + ui->sbCount->value() - 1;

    table->setItem(idx, TAB_ADDRESS,
                   new QTableWidgetItem(QString::number(addr)));

    table->setItem(idx, TAB_DATA,
                   new QTableWidgetItem(QString::number(0)));

    QString dataType = getDataTypeName(mb_func[ui->cbFunc->currentText()]);

    table->setItem(idx, TAB_DATA_TYPE,
                   new QTableWidgetItem(dataType));
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::delTableRow(QTableWidget *table)
{
    table->removeRow(table->rowCount() - 1);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
QString MainWindow::getDataTypeName(int mb_func) const
{
    QString type = "Unknown";

    switch (mb_func)
    {
    case MB_FUNC_READ_COILS:
    case MB_FUNC_WRITE_COIL:
    case MB_FUNC_WRITE_MULTIPLE_COILS:

        type = "Coil";

        break;

    case MB_FUNC_READ_DISCRETE_INPUT:

        type = "Discrete Input";

        break;

    case MB_FUNC_READ_INPUT_REGISTERS:

        type = "Input Register";

        break;

    case MB_FUNC_READ_HOLDING_REGISTERS:
    case MB_FUNC_WRITE_HOLDING_REGISTER:
    case MB_FUNC_WRITE_MULTIPLE_REGISTERS:

        type = "Holding Register";

        break;

    default:

        break;
    }

    return type;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
abstract_request_t *MainWindow::getRequestData()
{
    // Get function code from user interface
    quint8 func = static_cast<quint8>(mb_func[ui->cbFunc->currentText()]);
    // Check request type (read or write data from slave)
    RequestType type = getRequestType(func);

    // Process request
    switch (type)
    {
    case REQ_READ:
    {
        // Setup request structure
        read_request_t *request = new read_request_t();
        request->id = static_cast<quint8>(ui->sbSlaveID->value());
        request->func = func;
        request->address = static_cast<quint16>(ui->sbAddress->value());
        request->count = static_cast<quint16>(ui->sbCount->value());

        return request;
    }

    case REQ_WRITE:
    {
        write_request_t *request = new write_request_t();
        request->id = static_cast<quint8>(ui->sbSlaveID->value());
        request->func = func;
        request->address = static_cast<quint16>(ui->sbAddress->value());
        request->count = static_cast<quint16>(ui->sbCount->value());

        // Setup data to request data field
        for (int i = 0; i < request->count; i++)
            request->data[i] = static_cast<quint16>(ui->tableData->item(i, TAB_DATA)->text().toInt());

        return request;
    }

    default:

        statusPrint("ERROR: Unknown requst type");

        return nullptr;
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Check sender thread state
    if (threadCyclicSend.isRunning())
    {
        // Stop thread
        is_send_started = false;
        // Close window after terminating of sender thread
        is_close_event = true;
        // Ignore close event
        event->ignore();
    }
    else
        event->accept();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::onConnectRelease()
{
    if (master->isConnected())
    {
        master->closeConnection();
        ui->bConnect->setText("Connect");
    }
    else
    {
        // Get port settings from form controls
        serial_config_t serial_config = getSerialConfig();

        // Master init
        if (!master->init(serial_config))
        {
            statusPrint("ERROR: Master device initialization failed");
            return;
        }

        // Try connection
        master->openConnection();

        ui->bConnect->setText("Disconnect");
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::onRawDataClean()
{
    ui->ptRawData->clear();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::statusPrint(QString msg)
{
    ui->statusBar->clearMessage();
    ui->statusBar->showMessage(msg);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::changeDataTableRowsCount(int i)
{
    if (i > ui->tableData->rowCount())
        addTableRow(ui->tableData);

    if (i < ui->tableData->rowCount())
        delTableRow(ui->tableData);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::changeAddress(int i)
{
    // Calculate number of additional data table rows
    int delta = i - ui->tableData->item(0, TAB_ADDRESS)->text().toInt();

    // Update all data addresses in table
    for (int j = 0; j < ui->tableData->rowCount(); j++)
    {
        int addr = ui->tableData->item(j, TAB_ADDRESS)->text().toInt();
        addr += delta;

        QTableWidgetItem *item = new QTableWidgetItem;

        item->setText(QString::number(addr));

        ui->tableData->setItem(j, TAB_ADDRESS,item);
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::changedFunc(QString text)
{
     for (int j = 0; j < ui->tableData->rowCount(); j++)
    {
        QString dataType = getDataTypeName(mb_func[text]);

        ui->tableData->setItem(j, TAB_DATA_TYPE,
                               new QTableWidgetItem(dataType));
     }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::sendButtonRelease()
{
    // Check connection state
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }

    // Check sender thread state
    if (!threadCyclicSend.isRunning())
    {
        // Set started flag
        is_send_started = true;

        // Init data sender
        dataSender.init(is_cyclic, ui->sbSendInterval->value(), *getRequestData());

        // Move data sender to thread
        dataSender.moveToThread(&threadCyclicSend);

        // Connect start signal with thread function
        connect(&threadCyclicSend, &QThread::started,
                &dataSender, &DataSender::cyclicDataSend);

        // Start sender thread
        threadCyclicSend.start();

        // Mark button as stop button
        ui->bSend->setText("Stop");
    }
    else
    {
        // Reset started flag
        is_send_started = false;
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::buildButtonRelease()
{
    static bool flagChecked = false;
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }
    sendCoil(8,flagChecked);
    if(flagChecked)
    {
        //ui->build_schemes->setText("Собрать схему(on)");
    }
    else
    {
       // ui->build_schemes->setText("Собрать схему(off)");
    }
}

void MainWindow::destroyButtonRelease()
{
    static bool flagChecked = false;
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }
    sendCoil(7,flagChecked);
    if(flagChecked)
    {
       // ui->destroy_schemes->setText("Разобрать схему(on)");
    }
    else
    {
        //ui->destroy_schemes->setText("Разобрать схему(off)");
    }
}
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::onSlaveAnswer(answer_request_t answer)
{
    if (getRequestType(answer.func) == REQ_READ)
    {
        switch (_numCmd) {
        case DATAS_READ_HZ::DI:
            ui->checkBox_DI1->setChecked(answer.data[0] & 1);
            ui->checkBox_DI2->setChecked(answer.data[0] & 1 << 1);
            ui->checkBox_DI3->setChecked(answer.data[0] & 1 << 2);
            ui->checkBox_DI4->setChecked(answer.data[0] & 1 << 3);
            ui->checkBox_DI5->setChecked(answer.data[0] & 1 << 4);
            ui->checkBox_DI6->setChecked(answer.data[0] & 1 << 5);
            ui->checkBox_DI7->setChecked(answer.data[0] & 1 << 6);
            ui->checkBox_DI8->setChecked(answer.data[0] & 1 << 7);

            break;
        case DATAS_READ_HZ::AI:
            ui->label_ADCInA0_val->setText(QString::number(answer.data[0]));
            ui->label_ADCInA1_val->setText(QString::number(answer.data[1]));
            ui->label_ADCInA2_val->setText(QString::number(answer.data[2]));
            ui->label_ADCInA3_val->setText(QString::number(answer.data[3]));
            ui->label_ADCInA4_val->setText(QString::number(answer.data[4]));
            ui->label_ADCInA5_val->setText(QString::number(answer.data[5]));


            ui->label_ADCInB0_val->setText(QString::number(answer.data[6]));
            ui->label_ADCInB1_val->setText(QString::number(answer.data[7]));
            ui->label_ADCInB2_val->setText(QString::number(answer.data[8]));
            ui->label_ADCInB3_val->setText(QString::number(answer.data[9]));
            ui->label_ADCInB4_val->setText(QString::number(answer.data[10]));
            ui->label_ADCInB5_val->setText(QString::number(answer.data[11]));
            break;
        case DATAS_READ_HZ::DI_ERROR:
            ui->checkBox->setChecked(answer.data[0] & 1);
            ui->checkBox_2->setChecked(answer.data[0] & 1 << 1);
            ui->checkBox_3->setChecked(answer.data[0] & 1 << 2);
            ui->checkBox_4->setChecked(answer.data[0] & 1 << 3);
            ui->checkBox_5->setChecked(answer.data[0] & 1 << 4);
            ui->checkBox_6->setChecked(answer.data[0] & 1 << 5);
            break;
        default:
            break;
        }
        // Put received data into data table
        for (int i = 0; i < answer.count; i++)
        {
            ui->tableData->setItem(i, TAB_DATA,
                                   new QTableWidgetItem(QString::number(answer.data[i])));

            if(index == 0)
            {
                //ui->speed_val->setText(QString::number(answer.data[i]));
            }
            if(index == 1)
            {
             //   ui->power_val_2->setText(QString::number(answer.data[i]));
            }
        }
    }

    // Put raw data into raw data log
    onRawDataReceive(answer.getRawData());
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::onRawDataReceive(QByteArray rawData)
{
    QString buff = "Received data: ";
    quint8 tmp = 0;

    // Convert raw data to hexadicemal form
    for (int i = 0; i < rawData.count(); i++)
    {
        tmp = static_cast<quint8>(rawData.at(i));
        buff += QString("%1 ").arg(tmp, 2, 16, QLatin1Char('0'));
    }    

    ui->ptRawData->appendPlainText(buff);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::updatePortsList()
{
    // Get list of available ports
    QList<QSerialPortInfo> info = QSerialPortInfo::availablePorts();

    if (currentSerialPorts != info.count())
    {
        // Clean ports list
        ui->cbPort->clear();

        // Update ports list
        for (int i = 0; i < info.count(); i++)
        {
            ui->cbPort->addItem(info.at(i).portName());
        }

        currentSerialPorts = info.count();
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::printMsg(QString msg)
{
    ui->ptRawData->appendPlainText(msg);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::checkCyclicSend(int state)
{
    is_cyclic = static_cast<bool>(state);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::onFinishSendThread()
{
    // Disconnect thread function
    disconnect(&threadCyclicSend, &QThread::started,
               &dataSender, &DataSender::cyclicDataSend);

    // Mark button as send button
    ui->bSend->setText("Send");

    // Check close flag
    if (is_close_event)
    {
        master->closeConnection();
        QApplication::quit();
    }
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MainWindow::getStartedFlag(bool *started)
{
    *started = is_send_started;
}

void MainWindow::sendCoil(int num, bool &flag)
{
    // Check connection state
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }

    // Check sender thread state
    if (!threadCyclicSend.isRunning())
    {

        // Set started flag
        is_send_started = true;

        abstract_request_t tmp;
        tmp.id = 1;
        tmp.address = num;
        tmp.func = QModbusPdu::WriteSingleCoil;
        tmp.count = 1;
        if(flag == false)
        {
            tmp.data[0] = 1;
            flag = true;
        }
        else
        {
            tmp.data[0] = 0;
            flag = false;
        }
        // Init data sender
        dataSender.init(false, ui->sbSendInterval->value(), tmp);

        // Move data sender to thread
        dataSender.moveToThread(&threadCyclicSend);

        // Connect start signal with thread function
        connect(&threadCyclicSend, &QThread::started,
                &dataSender, &DataSender::cyclicDataSend);

        // Start sender thread
        threadCyclicSend.start();

        // Mark button as stop button
        ui->bSend->setText("Stop");
    }
    else
    {
        // Reset started flag
        is_send_started = false;
    }
}

void MainWindow::readHolding(int num,int count, MainWindow::DATAS_READ_HZ numCmd)
{
    _numCmd = numCmd;
    // Check connection state
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }

    // Check sender thread state
    if (!threadCyclicSend.isRunning())
    {

        // Set started flag
        is_send_started = true;

        abstract_request_t tmp;
        tmp.id = 1;
        tmp.address = num;
        tmp.func = QModbusPdu::ReadHoldingRegisters;
        tmp.count = count;
        //tmp.data[0] = 1;
        // Init data sender
        dataSender.init(false, ui->sbSendInterval->value(), tmp);

        // Move data sender to thread
        dataSender.moveToThread(&threadCyclicSend);

        // Connect start signal with thread function
        connect(&threadCyclicSend, &QThread::started,
                &dataSender, &DataSender::cyclicDataSend);

        // Start sender thread
        threadCyclicSend.start();

        // Mark button as stop button
        ui->bSend->setText("Stop");
    }
    else
    {
        // Reset started flag
        is_send_started = false;
    }
}


void MainWindow::sendHolding(int num, int data)
{
    // Check connection state
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }

    // Check sender thread state
    if (!threadCyclicSend.isRunning())
    {

        // Set started flag
        is_send_started = true;

        abstract_request_t tmp;
        tmp.id = 1;
        tmp.address = num;
        tmp.func = QModbusPdu::WriteSingleRegister;
        tmp.count = 1;
        tmp.data[0] = data;
        // Init data sender
        dataSender.init(false, ui->sbSendInterval->value(), tmp);

        // Move data sender to thread
        dataSender.moveToThread(&threadCyclicSend);

        // Connect start signal with thread function
        connect(&threadCyclicSend, &QThread::started,
                &dataSender, &DataSender::cyclicDataSend);

        // Start sender thread
        threadCyclicSend.start();

        // Mark button as stop button
        ui->bSend->setText("Stop");


    }
    else
    {
        // Reset started flag
        is_send_started = false;
    }
}

void MainWindow::on_dist_contr_released()
{
    static bool flagChecked = false;
    if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }
    sendCoil(5,flagChecked);
    if(flagChecked)
    {
     //   ui->dist_contr->setText("Дистанционное упр(on)");
    }
    else
    {
   //     ui->dist_contr->setText("Дистанционное упр(off)");
    }
}

void MainWindow::on_local_ctrl_released()
{
    static bool flagChecked = false;
     if (!master->isConnected())
     {
         printMsg("ERROR: Device is not connected");
         return;
     }
     sendCoil(6,flagChecked);
     if(flagChecked)
     {
    //     ui->local_ctrl->setText("Местн упр(on)");
     }
     else
     {
   //      ui->local_ctrl->setText("Местн упр(off)");
     }

}

void MainWindow::on_start_released()
{

    static bool flagChecked = false;
    if (!master->isConnected())
    {
         printMsg("ERROR: Device is not connected");
         return;
    }
        sendCoil(9,flagChecked);
        if(flagChecked)
    {
//         ui->start->setText("Старт(on)");
    }
    else
    {
        //ui->start->setText("Старт(off)");
    }
}

void MainWindow::on_stop_released()
{
    static bool flagChecked = false;
    if (!master->isConnected())
    {
         printMsg("ERROR: Device is not connected");
         return;
    }
    sendCoil(10,flagChecked);
    if(flagChecked)
    {
       //  ui->stop->setText("Стоп(on)");
    }
    else
    {
        //ui->stop->setText("Стоп(off)");
    }
}

void MainWindow::on_kvitirovanie_released()
{
    bool flagChecked = false;
    if (!master->isConnected())
    {
         printMsg("ERROR: Device is not connected");
         return;
    }

    sendCoil(11,flagChecked);
}

void MainWindow::updateData()
{


    // Check connection state
 /*   if (!master->isConnected())
    {
        printMsg("ERROR: Device is not connected");
        return;
    }

    // Check sender thread state
    if (!threadCyclicSend.isRunning())
    {
        // Set started flag
        is_send_started = true;

        abstract_request_t tmp;
        tmp.id = 1;
        tmp.address = index;
        tmp.func = QModbusPdu::ReadHoldingRegisters;
        tmp.count = 2;

        // Init data sender
        dataSender.init(false, ui->sbSendInterval->value(), tmp);

        // Move data sender to thread
        dataSender.moveToThread(&threadCyclicSend);

        // Connect start signal with thread function
        connect(&threadCyclicSend, &QThread::started,
                &dataSender, &DataSender::cyclicDataSend);

        // Start sender thread
        threadCyclicSend.start();

        //ui->speed_val->setText(QString::number())
        // Mark button as stop button
        ui->bSend->setText("Stop");
    }
    else
    {
        // Reset started flag
        is_send_started = false;
    }*/

 /*   if(threadCyclicSend.isRunning())
    {
       QThread::msleep(500);
       is_send_started = false;
    }
    // Check sender thread state
    if (!threadCyclicSend.isRunning())
    {
        // Set started flag
        is_send_started = true;

        abstract_request_t tmp;
        tmp.id = 1;
        tmp.address = 5;
        tmp.func = QModbusPdu::ReadHoldingRegisters;
        tmp.count = 5;
        // Init data sender
        dataSender.init(false, ui->sbSendInterval->value(), tmp);

        // Move data sender to thread
        dataSender.moveToThread(&threadCyclicSend);

        // Connect start signal with thread function
        connect(&threadCyclicSend, &QThread::started,
                &dataSender, &DataSender::cyclicDataSend);

        // Start sender thread
        threadCyclicSend.start();

        //ui->speed_val->setText(QString::number())
        // Mark button as stop button
        ui->bSend->setText("Stop");
    }
    else
    {
        // Reset started flag
        is_send_started = false;
    }*/
    /*++index;
    if(index == 50)
    {
        index = 0;
    }*/
}

void MainWindow::on_update_dat_released()
{
    //sendRequest()
    abstract_request_t request;
    request.id = 1;
    request.func = QModbusPdu::ReadHoldingRegisters;
    request.count = 1;
    dataSender.init(false, ui->sbSendInterval->value(), request);
    //sendMasterRequest(&request);
    dataSender.cyclicDataSend();
}
/*
void MainWindow::index_inc_realise()
{
    index++;
    if(index == 50)
    {
        index = 0;
    }
    ui->sbAddress->setValue(index);
}
*/

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
   // ui->seted_speed->setText(QString::number(/*ui->horizontalSlider.g)*/value));
}

void MainWindow::on_set_new_speed_released()
{
   // sendHolding(4,ui->horizontalSlider->value());
}

void MainWindow::on_checkBox_DO1_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(1,check);
}

void MainWindow::on_checkBox_DO2_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(2,check);
}

void MainWindow::on_checkBox_DO3_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(3,check);

}

void MainWindow::on_checkBox_DO4_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(4,check);

}

void MainWindow::on_checkBox_DO5_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(5,check);

}

void MainWindow::on_checkBox_DO6_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(6,check);
}

void MainWindow::on_checkBox_DO7_clicked(bool checked)
{

    bool check = checked == false;

    sendCoil(7,check);
}

void MainWindow::on_checkBox_DO8_clicked(bool checked)
{
    bool check = checked == false;

    sendCoil(8,check);
}

void MainWindow::on_pushButton_readDI_released()
{
    readHolding(1,1,DATAS_READ_HZ::DI);
}

void MainWindow::on_pushButton_readAI_released()
{
    readHolding(2,12,DATAS_READ_HZ::AI);
}

void MainWindow::on_pushButton_pwm1_released()
{
    bool check = ui->pushButton_pwm1->isChecked() == false;

    if(ui->pushButton_pwm1->isChecked())
    {
        ui->pushButton_pwm1->setText("Отключить ШИМ1");
    }
    else
    {
        ui->pushButton_pwm1->setText("Втключить ШИМ1");
    }

    sendCoil(9,check);
}

void MainWindow::on_pushButton_pwm2_released()
{
    bool check = ui->pushButton_pwm2->isChecked() == false;

    if(ui->pushButton_pwm2->isChecked())
    {
        ui->pushButton_pwm2->setText("Отключить ШИМ2");
    }
    else
    {
        ui->pushButton_pwm2->setText("Втключить ШИМ2");
    }

    sendCoil(10,check);
}

void MainWindow::on_pushButton_pwm3_released()
{
    bool check = ui->pushButton_pwm3->isChecked() == false;

    if(ui->pushButton_pwm3->isChecked())
    {
        ui->pushButton_pwm3->setText("Отключить ШИМ3");
    }
    else
    {
        ui->pushButton_pwm3->setText("Втключить ШИМ3");
    }

    sendCoil(11,check);
}

void MainWindow::on_pushButton_pwm4_released()
{
    bool check = ui->pushButton_pwm4->isChecked() == false;

    if(ui->pushButton_pwm4->isChecked())
    {
        ui->pushButton_pwm4->setText("Отключить ШИМ4");
    }
    else
    {
        ui->pushButton_pwm4->setText("Втключить ШИМ4");
    }

    sendCoil(12,check);
}

void MainWindow::on_horizontalSlider_PWM1_sliderReleased()
{
    //qDebug() << QString::number(ui->horizontalSlider_PWM1->value());
    sendHolding(15,ui->horizontalSlider_PWM1->value());
}

void MainWindow::on_horizontalSlider_PWM2_sliderReleased()
{
    sendHolding(16,ui->horizontalSlider_PWM2->value());
}

void MainWindow::on_horizontalSlider_PWM3_sliderReleased()
{
    sendHolding(17,ui->horizontalSlider_PWM3->value());
}

void MainWindow::on_horizontalSlider_PWM4_sliderReleased()
{
    sendHolding(18,ui->horizontalSlider_PWM4->value());
}

void MainWindow::on_pushButton_released()
{
    readHolding(19,1,DATAS_READ_HZ::DI_ERROR);
}
