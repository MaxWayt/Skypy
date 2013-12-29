#include "widgetchatwindow.h"
#include "widgetcontactslist.h"
#include "widgetchattab.h"
#include <iostream>
#include "clientmgr.h"
#include "audiomanager.h"
#include "networkmgr.h"

WidgetChatWindow::WidgetChatWindow(QWidget *parent) :
    QDialog(parent),
    Ui::WidgetChatWindow()
{
    setupUi(this);
}

WidgetChatTab* WidgetChatWindow::getChatTab(quint32 id)
{
    int size = _chatTab->count();
    for (int i = 0; i < size; ++i)
        if (WidgetChatTab* tab = dynamic_cast<WidgetChatTab*>(_chatTab->widget(i)))
            if (tab->getTabId() == id)
                return tab;
    return NULL;
}


WidgetChatTab* WidgetChatWindow::addChatTab(ContactInfo const* info, bool selectIt)
{
    WidgetChatTab* tab = getChatTab(info->getId());
    if (!tab)
    {
        tab = new WidgetChatTab(info, this);
        _chatTab->addTab(tab, info->getName());
    }
    if (selectIt)
        _chatTab->setCurrentWidget(tab);
    return tab;
}


void WidgetChatWindow::addMessageFrom(ContactInfo const* info, QString const& msg, bool notif)
{
    WidgetChatTab* tab = getChatTab(info->getId());
    if (!tab)
        tab = addChatTab(info, false);
    tab->addMessage(info->getId(), msg, notif);
}

void WidgetChatWindow::loginContact(quint32 id)
{
    if (WidgetChatTab* tab = getChatTab(id))
        tab->loginContact(id);
}

void WidgetChatWindow::logoutContact(quint32 id)
{
    if (WidgetChatTab* tab = getChatTab(id))
        tab->logoutContact(id);

    if (sClientMgr->getCallRequestPeerId() == id ||
            sClientMgr->getActiveCallPeerId() == id)
    {
        sClientMgr->setCallRequestPeerId(0);
        sClientMgr->setActiveCallPeerId(0);
        sAudioManager->quit();
        sNetworkMgr->quitCall();
    }

}

void WidgetChatWindow::handleCallResponse(SipRespond const& resp)
{
    if (WidgetChatTab* tab = getChatTab(resp.getDestId()))
        tab->handleCallResponse(resp);
}

void WidgetChatWindow::handleCallRequest(ContactInfo const* info, SipRequest const& req)
{
    WidgetChatTab* tab = getChatTab(info->getId());
    if (!tab)
        tab = addChatTab(info, false);
    tab->handleCallRequest(req);
}

void WidgetChatWindow::handleByeResponse(SipRespond const& resp)
{

}

void WidgetChatWindow::handleByeRequest(ContactInfo const* info, SipRequest const& req)
{
    sClientMgr->setCallRequestPeerId(0);
    sClientMgr->setActiveCallPeerId(0);
    sAudioManager->quit();
    sNetworkMgr->quitCall();

    WidgetChatTab* tab = getChatTab(info->getId());
    if (!tab)
        tab = addChatTab(info, false);
    tab->handleByeRequest(req);


    SipRespond Rep(200, req);
    sNetworkMgr->tcpSendPacket(Rep.getPacket());
}
