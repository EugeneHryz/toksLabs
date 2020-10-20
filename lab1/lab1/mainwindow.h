#pragma once

#include <QMainWindow>
#include <windows.h>
#include <fileapi.h>
#include <QDebug>
#include <QString>
#include <cmath>
#include <iostream>
#include <bitset>
#include <QRandomGenerator>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    int InitPorts(int baudRate);

    void DisplayControlSettings();

    QString PerformBitStuffing(const QString& data) const;
    QString ExtractStuffedBits(const QString& stuffedData) const;

    QString EncodeMessage(const QString& message) const;
    QString DecodeMessage(const QString& encodedMessage) const;

    QString EncodeWithHammingCode(const QString& data) const;
    QString DecodeHammingCode(const QString& encodedData) const;

private slots:
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;

    const QString HEADER = "01111110";

    DCB dcb1;
    DCB dcb2;

    HANDLE commPort1;
    HANDLE commPort2;

};
