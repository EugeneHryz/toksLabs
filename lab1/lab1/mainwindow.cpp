#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {

    ui->setupUi(this);
    ui->plainTextEdit->setReadOnly(true);

    InitPorts(9600);
    DisplayControlSettings();
}

MainWindow::~MainWindow() {
    delete ui;
}

int MainWindow::InitPorts(int baudRate) {

    commPort1 = CreateFileA("COM1", GENERIC_READ | GENERIC_WRITE,
                           0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (commPort1 == INVALID_HANDLE_VALUE) { return GetLastError(); }

    ZeroMemory(&dcb1, sizeof(DCB));
    dcb1.DCBlength = sizeof(DCB);

    if (!GetCommState(commPort1, &dcb1)) { return GetLastError(); }

    dcb1.BaudRate = baudRate;
    dcb1.Parity = NOPARITY;
    dcb1.ByteSize = 8;
    dcb1.StopBits = ONESTOPBIT;

    if (!SetCommState(commPort1, &dcb1)) { return GetLastError(); }


    commPort2 = CreateFileA("COM2", GENERIC_READ | GENERIC_WRITE,
                            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (commPort2 == INVALID_HANDLE_VALUE) { return GetLastError(); }

    ZeroMemory(&dcb2, sizeof(DCB));
    dcb2.DCBlength = sizeof(DCB);

    if (!GetCommState(commPort2, &dcb2)) { return GetLastError(); }

    dcb2.BaudRate = baudRate;
    dcb2.Parity = NOPARITY;
    dcb2.ByteSize = 8;
    dcb2.StopBits = ONESTOPBIT;

    if (!SetCommState(commPort2, &dcb2)) { return GetLastError(); }

    return 0;
}

void MainWindow::DisplayControlSettings() {

    ui->baudRate->setText("Baud rate: " + QString::number(dcb1.BaudRate));
    if (dcb1.Parity == 0) {
        ui->parity->setText("Parity: NO");
    } else {
        ui->parity->setText("Parity: YES");
    }
    switch (dcb1.StopBits) {

    case 0:
        ui->stopBits->setText("Stop bits: 1");
        break;
    case 1:
        ui->stopBits->setText("Stop bits: 1.5");
        break;
    case 2:
        ui->stopBits->setText("Stop bits: 2");
        break;
    }
    ui->byteSize->setText("Byte size: " + QString::number(dcb1.ByteSize));
}

QString MainWindow::PerformBitStuffing(const QString &data) const {

    QString resultData(data);
    QStringRef headerSubstr(&HEADER, 0, HEADER.size() - 1);

    int i = 0, matchPosition = 0;
    while (matchPosition != -1) {

        matchPosition = resultData.indexOf(headerSubstr, i);
        if (matchPosition != -1) {

            resultData.insert(matchPosition + headerSubstr.size(), QChar('1'));
            i += matchPosition + headerSubstr.size() + 1;
        }
    }

    return resultData;
}

QString MainWindow::ExtractStuffedBits(const QString &stuffedData) const {

    QString resultData(stuffedData);
    QString compareStr(HEADER);
    compareStr.replace(compareStr.size() - 1, 1, QChar('1'));

    int i = 0, matchPosition = 0;
    while (matchPosition != -1) {

        matchPosition = resultData.indexOf(compareStr.constData(), i);
        if (matchPosition != -1) {

            resultData.remove(matchPosition + compareStr.size() - 1, 1);
            i += matchPosition + compareStr.size() - 1;
        }
    }

    return resultData;
}

QString MainWindow::EncodeMessage(const QString &message) const {

    QString result;
    unsigned char bitMask = 128;

    for (int i = 0; i < message.size(); i++) {

        char c = message.at(i).toLatin1();
        while (bitMask != 0) {

            if (bitMask & c) {
                result.append("1");
            } else
                result.append("0");

            bitMask = bitMask >> 1;
        }

        bitMask = 128;
    }

    return result;
}

QString MainWindow::DecodeMessage(const QString &encodedMessage) const {

    QString result;
    unsigned char bitMask = 128;
    unsigned char accum = 0;

    int i = 0;
    while (i < encodedMessage.size()) {

        while (bitMask != 0) {

            if (encodedMessage.at(i) == QChar('1')) {
                accum = accum | bitMask;
            }

            bitMask = bitMask >> 1;
            i++;
        }

        result.append(QChar(accum));
        bitMask = 128;
        accum = 0;
    }

    return result;
}

void MainWindow::on_pushButton_clicked() {

    DWORD bytesWritten = 0;
    QString message = ui->textEdit->toPlainText();

    if (message.isEmpty()) { return; }

    QString dataField = PerformBitStuffing(EncodeMessage(message));
    QString packet = HEADER + dataField;

    if (WriteFile(commPort1, packet.toLocal8Bit().constData(),
                  packet.size(), &bytesWritten, NULL)) {

        ui->plainTextEdit->appendPlainText("Sended: " + message.left(bytesWritten));
        packet.insert(HEADER.size() - 1, "   ");
        ui->plainTextEdit->appendPlainText("Packet: " + packet);

    }

    char *readBuffer = new char[bytesWritten + 1];
    DWORD bytesRead = 0;

    if (ReadFile(commPort2, readBuffer, bytesWritten, &bytesRead, NULL)) {

        readBuffer[bytesRead] = '\0';

        QString recievedPacket(readBuffer);
        QString recievedMessage;
        if (recievedPacket.startsWith(HEADER)) {

            recievedMessage = DecodeMessage(ExtractStuffedBits(recievedPacket.mid(HEADER.size())));
        }

        ui->plainTextEdit->appendPlainText("\nRecieved: " + QString(recievedMessage) + "\n");
        ui->textEdit->clear();
    }

    delete []readBuffer;
}
