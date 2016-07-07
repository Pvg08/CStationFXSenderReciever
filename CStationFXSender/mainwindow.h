#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "serialfxwriter.h"

#include "datageneratorledscreen.h"
#include "datageneratorledring.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_start_clicked();

    void transaction();
    void showResponse(const QString &s);
    void processError(const QString &s);
    void processTimeout(const QString &s);
    void processLog(const QString &s);

    void on_toolButton_refresh_clicked();

    void on_checkBox_detailed_clicked(bool checked);

private:
    Ui::MainWindow *ui;
    int transactionCount;

    SerialFXWriter thread;
    DataGenerator* generator;
};

#endif // MAINWINDOW_H
