#include "widgetchattab.h"
#include "widgetcontactslist.h"
#include "packet.hpp"
#include "opcodemgr.h"
#include "networkmgr.h"
#include "sipPacket.hpp"
#include "audiomanager.h"
#include "clientmgr.h"
#include <QMessageBox>

WidgetChatTab::WidgetChatTab(ContactInfo const* info, QWidget *parent) :
    QWidget(parent),
    Ui::WidgetChatTab(),
    _tabId(info->getId()),
    _peersMap(),
    _tabType(CHAT_TAB_SINGLE)
    //_peerId(info->getId()), _peerName(info->getName()), _peerEmail(info->getEmail()),
    //_peerPublicIp(info->getPublicIp()), _peerPrivateIp(info->getPrivateIp()),
    //_online(true)
{
    setupUi(this);

    PeerInfo* peer = new PeerInfo();
    peer->peerId = info->getId();
    peer->peerEmail = info->getEmail();
    peer->peerName = info->getName();
    peer->peerPublicIp = info->getPublicIp();
    peer->peerPrivateIp = info->getPrivateIp();
    peer->online = true;
    _peersMap[peer->peerId] = peer;
}

void WidgetChatTab::on__callButon_clicked()
{
    if (!_getOnlinePeerCount() > 0)
        return;
    if (_tabType == CHAT_TAB_SINGLE)
    {
        PeerInfo const* peer = _getFirstPeer();
        std::cout << "CLICKED CALL BUTTON" << std::endl;
        if (sClientMgr->getActiveCallPeerId() == peer->peerId || sClientMgr->getCallRequestPeerId() == peer->peerId)
        {
            sClientMgr->stopCall(peer->peerEmail, peer->peerId, peer->peerPublicIp, peer->peerPrivateIp);
            _callButon->setText("Call");
        }
        else
            sClientMgr->makeCall(peer->peerEmail, peer->peerId, peer->peerPublicIp, peer->peerPrivateIp);
    }
}

void WidgetChatTab::on__inputText_returnPressed()
{
    QString text = _inputText->text().trimmed();
    if (text.size() == 0)
        return;

    if (_tabType == CHAT_TAB_SINGLE)
    {
        Packet pkt(CMSG_CHAT_TEXT);
        pkt << quint32(_getFirstPeer()->peerId);
        pkt << text;
        sNetworkMgr->tcpSendPacket(pkt);
    }

    _inputText->setText("");
    _chatTable->addItem(sClientMgr->getUserName() + ": " + text);
    _chatTable->scrollToBottom();
}

void WidgetChatTab::on__sendButton_clicked()
{
    on__inputText_returnPressed();
}

quint32 WidgetChatTab::_getOnlinePeerCount() const
{
    quint32 count = 0;
    for (QMap<quint32, PeerInfo*>::ConstIterator itr = _peersMap.begin(); itr != _peersMap.end(); ++itr)
        if (itr.value()->online)
            ++count;
    return count;
}

WidgetChatTab::PeerInfo* WidgetChatTab::_getPeerInfo(quint32 id)
{
    QMap<quint32, PeerInfo*>::Iterator itr = _peersMap.find(id);
    if (itr == _peersMap.end())
        return NULL;
    return itr.value();
}

WidgetChatTab::PeerInfo const* WidgetChatTab::_getPeerInfo(quint32 id) const
{
    QMap<quint32, PeerInfo*>::ConstIterator itr = _peersMap.find(id);
    if (itr == _peersMap.end())
        return NULL;
    return itr.value();
}

WidgetChatTab::PeerInfo const* WidgetChatTab::_getFirstPeer() const
{
    QMap<quint32, PeerInfo*>::ConstIterator itr = _peersMap.begin();
    if (itr != _peersMap.end())
        return itr.value();
    return NULL;
}

void WidgetChatTab::loginContact(quint32 id)
{
    PeerInfo* peer = _getPeerInfo(id);
    if (!peer)
        return;
    peer->online = true;

    _callButon->setEnabled(true);
    _sendButton->setEnabled(true);

    addMessage(peer->peerName + " logged in !");
}

void WidgetChatTab::logoutContact(quint32 id)
{
    PeerInfo* peer = _getPeerInfo(id);
    if (!peer)
        return;
    peer->online = false;
    bool enable = _getOnlinePeerCount() > 0;
    _callButon->setEnabled(enable);
    if (enable)
        _callButon->setText("Call");
    _sendButton->setEnabled(enable);

    addMessage(peer->peerName + " logged out ...");
}

void WidgetChatTab::addMessage(quint32 id, QString const& msg, bool notif)
{
    QString item = msg;
    if (!notif)
    {
        if (PeerInfo const* peer = _getPeerInfo(id))
            item = peer->peerName + ": " + msg;
    }
    _chatTable->addItem(item);
    _chatTable->scrollToBottom();
}

void WidgetChatTab::addMessage(QString const& msg)
{
    QString item = msg;
    _chatTable->addItem(item);
    _chatTable->scrollToBottom();
}

void WidgetChatTab::handleCallResponse(SipRespond const& resp)
{
    switch (resp.getCode())
    {
    case 100: // Forward de l'appel
        addMessage("Send call request ...");
        _callButon->setText("Stop");
        break;
    case 404: // Contact non connecte
        addMessage(resp.getDestEmail() + " isn't online");
        sClientMgr->setCallRequestPeerId(0);
        sNetworkMgr->quitCall();
        _callButon->setText("Call");
        break;
    case 180: // Ca sonne
        addMessage("Ringing ...");
        break;
    case 200: // Ca a decroche
        addMessage("Call accepted");
        sClientMgr->setCallRequestPeerId(0);
        if (sAudioManager->start())
        {
            std::cout << "RECEIV PEER ADDR: " << resp.getDestIp().toStdString() << std::endl;
            sNetworkMgr->setCallPeerAddr(QHostAddress(resp.getDestIp()), resp.getDestPort());
            sNetworkMgr->runCall();
            sClientMgr->setActiveCallPeerId(resp.getDestId());
        }
        else
            std::cout << "FAIL TO START AUDIO" << std::endl;
        break;
    case 603: // Refuse
        addMessage("Call refused");
        sClientMgr->setCallRequestPeerId(0);
        sNetworkMgr->quitCall();
        _callButon->setText("Call");
        break;
    case 604: // Occuped
        addMessage("Occuped");
        sClientMgr->setCallRequestPeerId(0);
        sNetworkMgr->quitCall();
        _callButon->setText("Call");
        break;
    case 605: // Peer fail to open network
        addMessage("Peer's network issue");
        sClientMgr->setCallRequestPeerId(0);
        sNetworkMgr->quitCall();
        _callButon->setText("Call");
        break;
    case 606: // Peer fail to start audio
        addMessage("Peer's audio issue");
        sClientMgr->setCallRequestPeerId(0);
        sNetworkMgr->quitCall();
        _callButon->setText("Call");
        break;
    }
}

void WidgetChatTab::handleCallRequest(SipRequest const& request)
{
    addMessage("Incomming call...");
    sClientMgr->setCallRequestPeerId(request.getSenderId());
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Incomming call", "Accept call from " + request.getSenderEmail() + " ?",
                              QMessageBox::Yes | QMessageBox::No);
    switch (reply)
    {
        case QMessageBox::Yes:
        case QMessageBox::No:
        {
            QHostAddress host(request.getDestIp());
            if (reply == QMessageBox::Yes)
            {
                for (quint16 selfPort = request.getSenderPort() + 1; selfPort < request.getSenderPort() + 200; ++selfPort)
                {
                    if (sNetworkMgr->setCallHostAddr(host, selfPort))
                    {
                        if (sAudioManager->start())
                        {
                            std::cout << "SET PEER ADDR: " << request.getSenderIp().toStdString() << std::endl;
                            sNetworkMgr->setCallPeerAddr(QHostAddress(request.getSenderIp()), request.getSenderPort());
                            sNetworkMgr->runCall();

                            std::cout << "CALL ACCEPTED, LISTEN ON " << request.getDestIp().toStdString() << ":" << request.getDestPort() << std::endl;
                            SipRespond Rep(200, request, selfPort);
                            sNetworkMgr->tcpSendPacket(Rep.getPacket());
                            sClientMgr->setCallRequestPeerId(0);
                            sClientMgr->setActiveCallPeerId(request.getSenderId());
                            _callButon->setText("Stop");
                            return;
                        }
                        else // Should send error
                        {
                            std::cout << "FAIL TO START AUDIO" << std::endl;

                            SipRespond Rep(606, request);
                            sNetworkMgr->tcpSendPacket(Rep.getPacket());
                            sClientMgr->setCallRequestPeerId(0);
                        }
                    }
                }

                // Call not succeded
                {
                    std::cout << "FAIL TO OPEN NETWORK: " << request.getDestIp().toStdString() << ":" << request.getDestPort() << std::endl;

                    SipRespond Rep(605, request);
                    sNetworkMgr->tcpSendPacket(Rep.getPacket());
                    sClientMgr->setCallRequestPeerId(0);
                }
            }
            else
            {
                SipRespond Rep(603, request);
                sNetworkMgr->tcpSendPacket(Rep.getPacket());
                sClientMgr->setCallRequestPeerId(0);
                std::cout << "SEND CALL REFUSED" << std::endl;
            }
            break;
        }
        default:
            sClientMgr->setCallRequestPeerId(0);
            break;
    }
}

void WidgetChatTab::handleByeRequest(SipRequest const& req)
{
    _callButon->setText("Call");
    if (PeerInfo const* peer = _getPeerInfo(req.getSenderId()))
        addMessage(peer->peerName + " close call");
}
