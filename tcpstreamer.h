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
#ifndef TCPSTREAMER_H
#define TCPSTREAMER_H

#include <QObject>
#include <QThread>
#include <QTcpServer>
#include <QMutex>

namespace QTcpDebug {

class TcpStreamer : public QObject
{
    Q_OBJECT
public:
    static TcpStreamer *instance();

    static void tcpMessageHandler(QtMsgType, const QMessageLogContext &,
                                  const QString &);

private:
    explicit TcpStreamer(QObject *parent = nullptr);

    void enableMessageHandler();
    void disableMessageHandler();

    void appendMessage(const QString &message);

signals:
    void newMessage();

private slots:
    void onClientDisconnected();

private:
    QThread m_debugStreamThread;
    QTcpServer *m_tcpServer;
    QList<QTcpSocket*> m_clients;

    QMutex m_messageMutex;
    QStringList m_messages;

    bool m_messageHandlerActive;

    static TcpStreamer *m_instance;
};

}

#endif // TCPSTREAMER_H
