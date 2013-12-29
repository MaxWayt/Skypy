#include <QMessageBox>
#include <QRegExpValidator>
#include <QHostInfo>

#include "mainwindow.h"
#include "networkmgr.h"

#include "packet.hpp"
#include "opcodemgr.h"

#include "audiomanager.h"
#include "widgetaddcontactwindow.h"

#include <iostream>

#include "sipPacket.hpp"
#include "clientmgr.h"

#include <QNetworkInterface>

MainWindow::MainWindow(QMainWindow *parent) :
    QMainWindow(parent),
    _widgets(new QStackedWidget(this)),
    _loginForm(new WidgetLoginForm(this)),
    _contactForm(new WidgetContactsList(this))
{
    setCentralWidget(_widgets);
    _widgets->addWidget(_loginForm);
    _widgets->addWidget(_contactForm);
    _loginForm->initialize();
    _widgets->setCurrentWidget(_loginForm);

    sNetworkMgr->setMainWindow(this);

    // Init audio
    if (!sAudioManager->setInputDevice(DEFAULT_DEVICE, AudioSample::MONO, AudioSample::FREQ_48000))
        std::cout << "FAIL INIT AUDIO INPUT: " << sAudioManager->errorText().toStdString() << std::endl;
    if (!sAudioManager->setOutputDevice(DEFAULT_DEVICE, AudioSample::MONO, AudioSample::FREQ_48000))
        std::cout << "FAIL INIT AUDIO OUPUT: " << sAudioManager->errorText().toStdString() << std::endl;
}

MainWindow::~MainWindow()
{
}

void MainWindow::handleRequireAuth(QString const& localAddr)
{
    Packet pkt(CMSG_AUTH);
    pkt << _loginForm->getEmailText();
    pkt << _loginForm->getPasswordText();
    pkt << localAddr;
    sNetworkMgr->tcpSendPacket(pkt);
    std::cout << "AUTH SENDED" << std::endl;
    std::cout << "LOCAL IP: " << localAddr.toStdString() << std::endl;
    pkt.dumpHex();
}

bool MainWindow::handleAuthResult(Packet& pkt)
{
    quint8 result;
    pkt >> result;

    std::cout << "AUTH RESULT: " << quint32(result) << std::endl;

    if (result == 0)
    {
        _loginForm->unload();
        _contactForm->initialize();
        _widgets->setCurrentWidget(_contactForm);
    }
    else
        QMessageBox::information(this, "Authentification", "Fail to authenticate");

    return (result == 0);
}

void MainWindow::handleServerConnectionLost(QAbstractSocket::SocketError e, QString const& msg)
{
    (void)e;
    _contactForm->unload();
    _loginForm->initialize();
    setWindowTitle("");
    _widgets->setCurrentWidget(_loginForm);
    QMessageBox::information(this, "Connection error", "Error: " + msg);
}

void MainWindow::handleContactLogin(Packet& pkt)
{
    quint32 count;
    pkt >> count;
    std::cout << "RECEIV CONTACT COUNT: " << count << std::endl;
    for (quint32 i = 0; i < count; ++i)
    {
        quint32 id;
        QString name;
        QString email;
        QString ipPublic, ipPrivate;
        pkt >> id;
        pkt >> name;
        pkt >> email;
        pkt >> ipPublic;
        pkt >> ipPrivate;

        ContactInfo* info = new ContactInfo(_contactForm->getContactListWidget(), id, name, email, true, ipPublic, ipPrivate);
        _contactForm->loginContact(info);
    }
}

void MainWindow::handleContactLogout(Packet& pkt)
{
    quint32 count;
    pkt >> count;
    for (quint32 i = 0; i < count; ++i)
    {
        quint32 id;
        pkt >> id;
        _contactForm->logoutContact(id);
    }
}

void MainWindow::handleChatText(Packet &pkt)
{
    quint32 from;
    QString msg;
    pkt >> from;
    pkt >> msg;
    std::cout << "RECEIV MSG FROM: " << from << " - " << msg.toStdString() << std::endl;
    _contactForm->addMessageFrom(from, msg);
}

void MainWindow::handleSearchContactResult(Packet &pkt)
{
    quint32 count;
    pkt >> count;

    _contactForm->getContactWindow()->getResultListWidget()->clear();
    for (quint32 i = 0; i < count; ++i)
    {
        QString name;
        QString email;
        quint8 online;
        pkt >> name >> email >> online;
        ContactInfo* info = new ContactInfo(_contactForm->getContactWindow()->getResultListWidget(), 0, name, email, online != 0);
        info->setText(name + " (" + email + ")");
        _contactForm->getContactWindow()->addResult(info);
    }
}

void MainWindow::handleAddContactRequest(Packet &pkt)
{
    quint32 reqId;
    QString name;
    QString email;
    pkt >> reqId >> name >> email;

    ContactInfo* info = new ContactInfo(NULL, reqId, name, email, false);
    info->setText(name + " (" + email + ")");

    Notification* notif = new Notification(_contactForm->getNotificationWidget(), NOTIF_CONTACT_REQUEST, "New contact request from " + info->text(), info);
    _contactForm->getNotificationWidget()->addItem(notif);
}

void MainWindow::handleSipRep(Packet &pkt)
{
  sNetworkMgr->handleSipRep(pkt);
}

void MainWindow::handleSipRequest(Packet &pkt)
{
  sNetworkMgr->handleSipRequest(pkt);
}

void MainWindow::handlesipResponse(SipRespond const& resp)
{
    if (resp.getCmd() == "INVITE")
    {
        _contactForm->handleCallResponse(resp);
    }
    else if (resp.getCmd() == "BYE")
        _contactForm->handleByeResponse(resp);
}

void MainWindow::handleCallRequest(SipRequest const& request)
{
    ContactInfo* sender = sClientMgr->findContact(request.getSenderId());
    if (!sender || sClientMgr->hasActiveCall() || sClientMgr->hasCallRequest())
    {

        SipRespond Rep(604, request);
        sNetworkMgr->tcpSendPacket(Rep.getPacket());
        return;
    }

    _contactForm->handleCallRequest(sender, request);
}

void MainWindow::handleByeRequest(const SipRequest &request)
{
    ContactInfo* sender = sClientMgr->findContact(request.getSenderId());
    if (!sender || sClientMgr->getActiveCallPeerId() != request.getSenderId())
    {

        SipRespond Rep(604, request);
        sNetworkMgr->tcpSendPacket(Rep.getPacket());
        return;
    }

    _contactForm->handleByeRequest(sender, request);

}

void MainWindow::handleAccountInfo(Packet& pkt)
{
    quint32 id;
    QString name, email, publicIp;
    pkt >> id >> name >> email >> publicIp;
    std::cout << "RECEIV id: " << id << " - name: " << name.toStdString() << " - email: " << email.toStdString() << " - publicIp: " << publicIp.toStdString() << std::endl;
    setWindowTitle(name + " (" + email + ")");
    sClientMgr->setAccountInfo(id, name, email);
    sClientMgr->setPublicIp(publicIp);
}
