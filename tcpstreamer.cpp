/*
MIT License

Copyright (c) 2019 Erik Larsson

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#include "tcpstreamer.h"

#include <iostream>
#include <QTcpSocket>
#include <QTextStream>
#include <QMutexLocker>

namespace QTcpDebug {

TcpStreamer *TcpStreamer::m_instance = nullptr;

TcpStreamer::TcpStreamer(QObject *parent)
    : QObject(parent)
    , m_messageHandlerActive(false)
{
    moveToThread(&m_debugStreamThread);
    m_debugStreamThread.setObjectName("DebugThread");
    m_debugStreamThread.start();


    m_tcpServer = new QTcpServer();
    m_tcpServer->listen(QHostAddress::Any, 6001);
    m_tcpServer->moveToThread(&m_debugStreamThread);


    connect(m_tcpServer, &QTcpServer::newConnection, [=]() {
        QTcpSocket *client = m_tcpServer->nextPendingConnection();

        if (client != nullptr) {
            connect(client, &QTcpSocket::disconnected,
                    this, &TcpStreamer::onClientDisconnected);
            m_clients.append(client);

            if(m_messageHandlerActive == false) {
                enableMessageHandler();
            }
        }
    });

    connect(this, &TcpStreamer::newMessage, this, [=]() {
        QMutexLocker l(&m_messageMutex);
        for (auto message : m_messages) {
            for (auto client : m_clients) {
                client->write(message.toUtf8());
            }
        }
        m_messages.clear();
    });

}

void TcpStreamer::tcpMessageHandler(QtMsgType type,
                                    const QMessageLogContext &message,
                                    const QString &text)
{
    QString data;

    QString typeStr;
    switch (type) {
    case QtDebugMsg:
        typeStr = "debug";
        break;
    case QtWarningMsg:
        typeStr = "warning";
        break;
    case QtCriticalMsg:
        typeStr = "critical";
        break;
    case QtFatalMsg:
        typeStr = "fatal";
        break;
    case QtInfoMsg:
        typeStr = "info";
        break;
    }

    QString file(message.file);
    QString line = QString("%1").arg(message.line);

    data.append(typeStr.leftJustified(9, ' '));
    data.append(file.split("/").last().leftJustified(15, ' '));
    data.append(line.rightJustified(5, ' '));
    data.append(" ");
    data.append(text);
    data.append("\r\n");

    instance()->appendMessage(data);
}

TcpStreamer *TcpStreamer::instance()
{
    if (m_instance == nullptr) {
        m_instance = new TcpStreamer();
    }
    return m_instance;
}

void TcpStreamer::enableMessageHandler()
{
    m_messageHandlerActive = true;
    qInstallMessageHandler(TcpStreamer::tcpMessageHandler);
}

void TcpStreamer::disableMessageHandler()
{
    m_messageHandlerActive = false;
    qInstallMessageHandler(nullptr);
}

void TcpStreamer::appendMessage(const QString &message)
{
    m_messageMutex.lock();
    m_messages.append(message);
    m_messageMutex.unlock();

    emit newMessage();
}

void TcpStreamer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client != nullptr) {
        if (m_clients.contains(client)) {
            m_clients.removeOne(client);
        }

        client->close();
        client->deleteLater();
    }

    if (m_clients.isEmpty()) {
        disableMessageHandler();
    }
}

}
