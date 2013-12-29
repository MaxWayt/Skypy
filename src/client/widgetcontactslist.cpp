#include "widgetcontactslist.h"
#include <QtGlobal>
#include <iostream>
#include "widgetchatwindow.h"
#include "widgetaddcontactwindow.h"
#include <QMessageBox>
#include "packet.hpp"
#include "opcodemgr.h"
#include "networkmgr.h"
#include "clientmgr.h"
#include "audiomanager.h"

WidgetContactsList::WidgetContactsList(QWidget *parent) :
    QWidget(parent),
    Ui::WidgetContactsList(),
    _chatWindow(new WidgetChatWindow(this)),
    _addContactWindow(new WidgetAddContactWindow(this))
{
    setupUi(this);
    QObject::connect(_contactList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleContactDoubleClick(QListWidgetItem*)));
    QObject::connect(_notificationList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleNotificationDoubleClick(QListWidgetItem*)));

    _addContactWindow->setModal(true);
}

void WidgetContactsList::initialize()
{
    sClientMgr->clearContacts();
    _contactList->clear();
    _notificationList->clear();
    sClientMgr->setAccountInfo(0, "", "");

    Packet data(CMSG_GET_ACCOUNT_INFO);
    sNetworkMgr->tcpSendPacket(data);
}

void WidgetContactsList::unload()
{
    _chatWindow->hide();
}

void WidgetContactsList::loginContact(ContactInfo* info)
{
    if (sClientMgr->addContact(info))
    {
        _chatWindow->loginContact(info->getId());
        _contactList->addItem(info);
    }
}

void WidgetContactsList::logoutContact(quint32 id)
{
    ContactInfo* info = sClientMgr->findContact(id);
    if (!info)
        return;

    _chatWindow->logoutContact(id);
    _contactList->removeItemWidget(info);
    sClientMgr->removeContact(id);

    if (sClientMgr->getCallRequestPeerId() == id || sClientMgr->getActiveCallPeerId() == id)
    {
        std::cout << "PEER " << id << " LOGOUT, STOPPING CALL" << std::endl;
        sClientMgr->setCallRequestPeerId(0);
        sClientMgr->setActiveCallPeerId(0);
        sAudioManager->quit();
        sNetworkMgr->quitCall();
    }
}

void WidgetContactsList::handleContactDoubleClick(QListWidgetItem* item)
{
    ContactInfo* contact = dynamic_cast<ContactInfo*>(item);
    std::cout << "CLICKED ON " << contact->getEmail().toStdString() << std::endl;

    if (contact)
    {
        _chatWindow->addChatTab(contact, true);
        _chatWindow->show();
    }
}


void WidgetContactsList::addMessageFrom(quint32 id, QString const& msg, bool notif)
{
    ContactInfo* info = sClientMgr->findContact(id);
    if (!info)
        return;

    std::cout << "MSG FROM: " << info->getEmail().toStdString() << " - " << msg.toStdString() << std::endl;
    _chatWindow->addMessageFrom(info, msg, notif);
}

void WidgetContactsList::on__addContactButton_clicked()
{
    _addContactWindow->show();
}

void WidgetContactsList::handleNotificationDoubleClick(QListWidgetItem* item)
{
    Notification* notif = dynamic_cast<Notification*>(item);
    if (!notif)
    {
        _notificationList->removeItemWidget(item);
        return;
    }

    switch (notif->getNotificationType())
    {
        case NOTIF_CONTACT_REQUEST:
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Contact request", "Accept contact request from " + notif->getSender()->text() + " ?",
                                      QMessageBox::Yes | QMessageBox::No);
            switch (reply)
            {
                case QMessageBox::Yes:
                case QMessageBox::No:
                {
                    Packet data(CMSG_ADD_CONTACT_RESPONSE);
                    data << quint32(notif->getSender()->getId()); // request id
                    data << quint8(reply == QMessageBox::Yes ? 1 : 0);
                    std::cout << (reply == QMessageBox::Yes ? "Accept" : "Refuse") << " contact request " << notif->getSender()->getId() << std::endl;
                    sNetworkMgr->tcpSendPacket(data);

                    _notificationList->removeItemWidget(notif);
                    delete notif;
                    break;
                }
            }
        }
    }

}

void WidgetContactsList::on__debugButton_clicked()
{
    sNetworkMgr->debugInput();
}
