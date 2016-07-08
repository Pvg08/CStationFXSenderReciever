#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtSerialPort/QSerialPortInfo>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    on_toolButton_refresh_clicked();

    //generator = new DataGeneratorLEDScreen();
    generator = new DataGeneratorLEDRing();

    generator->setBaseTimeout(ui->spinBox_timeout->value());
    transactionCount = 0;
    connect(&thread, SIGNAL(response(QString)), this, SLOT(showResponse(QString)));
    connect(&thread, SIGNAL(error(QString)), this, SLOT(processError(QString)));
    connect(&thread, SIGNAL(timeout(QString)), this, SLOT(processTimeout(QString)));
    connect(&thread, SIGNAL(log(QString)), this, SLOT(processLog(QString)));
    connect(&thread, SIGNAL(frame_play_confirmed()), this, SLOT(frame_played()));

    fps_counter = 0;
    millis = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

MainWindow::~MainWindow()
{
    if (thread.isRunning()) thread.terminate();
    delete generator;
    delete ui;
}

void MainWindow::transaction()
{
    ui->textEdit_log->append(tr("Status: Running, connected to port %1.").arg(ui->comboBox_port->currentText()));
    ui->statusBar->showMessage(tr("Status: Running, connected to port %1.").arg(ui->comboBox_port->currentText()), 10000);
    thread.listen(ui->comboBox_port->currentText(), 30000, generator);
}

void MainWindow::showResponse(const QString &s)
{
    ui->textEdit_log->append(tr("Traffic, transaction #%1:"
                                "\n-response: %3")
                             .arg(++transactionCount).arg(s));
}

void MainWindow::processError(const QString &s)
{
    ui->textEdit_log->append(tr("Status: Not running, %1.").arg(s));
    ui->statusBar->showMessage(tr("No traffic."), 5000);
}

void MainWindow::processTimeout(const QString &s)
{
    ui->textEdit_log->append(tr("Status: Running, %1.").arg(s));
    ui->statusBar->showMessage(tr("No traffic."), 5000);
}

void MainWindow::processLog(const QString &s)
{
    ui->textEdit_log->append(s);
}

void MainWindow::frame_played()
{
    fps_counter++;
    if (abs(QDateTime::currentDateTime().toMSecsSinceEpoch()-millis)>1000) {
        ui->label_freq->setText(QString::number(round(1000.0*fps_counter/(QDateTime::currentDateTime().toMSecsSinceEpoch()-millis))) + " fps");
        millis = QDateTime::currentDateTime().toMSecsSinceEpoch();
        fps_counter = 0;
    }
}

void MainWindow::on_pushButton_start_clicked()
{
    if (thread.isRunning()) {
        thread.do_stop();
        ui->pushButton_start->setText(tr("Start"));
    } else {
        transaction();
        ui->pushButton_start->setText(tr("Stop"));
    }
}

void MainWindow::on_toolButton_refresh_clicked()
{
    ui->comboBox_port->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboBox_port->addItem(info.portName());
    }
}

void MainWindow::on_checkBox_detailed_clicked(bool checked)
{
    if (checked) {
        connect(&thread, SIGNAL(log(QString)), this, SLOT(processLog(QString)));
    } else {
        disconnect(&thread, SIGNAL(log(QString)), this, SLOT(processLog(QString)));
    }
}

void MainWindow::on_spinBox_timeout_valueChanged(int arg1)
{
    generator->setBaseTimeout(arg1);
}
