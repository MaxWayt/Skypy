#ifndef CLIENTMGR_H
#define CLIENTMGR_H

#include "singleton.h"
#include <QString>
#include <QtCore>
#include <QtGlobal>
#include <QMap>
#include <QListWidgetItem>

class ContactInfo : public QListWidgetItem
{
public:
    ContactInfo(QListWidget* parent, quint32 id, QString const& name, QString const& email, bool online = true, QString const& ipPublic = "", QString const& ipPrivate = "") :
        QListWidgetItem(QString(name + " (" + email + ") - " + (online ? "Online" : "Offline")), parent),
        _id(id), _name(name), _email(email), _online(online), _publicIp(ipPublic), _privateIp(ipPrivate)
    {}
    ContactInfo(QListWidget *parent, ContactInfo const* info) :
        QListWidgetItem(QString(info->getName() + " (" + info->getEmail() + ") - " + (info->isOnline() ? "Online" : "Offline")), parent),
        _id(info->getId()), _name(info->getName()), _email(info->getEmail()), _online(info->isOnline()), _publicIp(info->getPublicIp()), _privateIp(info->getPrivateIp())
    {}

    quint32 getId() const { return _id; }
    QString const& getName() const { return _name; }
    QString const& getEmail() const { return _email; }
    bool isOnline() const { return _online; }
    QString const& getPublicIp() const { return _publicIp; }
    QString const& getPrivateIp() const { return _privateIp; }

private:
    quint32 _id;
    QString _name;
    QString _email;
    bool _online;
    QString _publicIp;
    QString _privateIp;
};

struct CallPeer
{
    CallPeer(quint32 chatId, quint32 id, QString const& email, QString const& ip, quint16 port, bool active = false) :
        chatId(chatId), id(id), email(email), callIp(ip), port(port), active(active)
    {}
    quint32 chatId;
    quint32 id;
    QString email;
    QString callIp;
    quint16 port;
    bool active;
};

class ClientMgr : public Singleton<ClientMgr>
{
public:
    ClientMgr();

    void setAccountInfo(quint32 id, QString const& name, QString const& email) { _accountId = id; _username = name; _email = email; }
    quint32 getAccountId() const { return _accountId; }
    QString const& getUserName() const { return _username; }
    QString const& getEmail() const { return _email; }
    QString const& getPublicIp() const { return _publicIp; }
    void setPublicIp(QString const& ip) { _publicIp = ip; }
    QString const& getPrivateIp() const { return _privateIp; }
    void setPrivateIp(QString const& ip) { _privateIp = ip; }

    bool hasActiveCall() const
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->active)
                return true;
        return false;
    }
    bool hasActiveCallWith(quint32 id) const
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->active && (*itr)->id == id)
                return true;
        return false;
    }
    void setActiveCallPeer(quint32 id)
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->id == id)
                (*itr)->active = true;
    }

    bool hasCallRequest() const
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if (!(*itr)->active)
                return true;
        return false;
    }
    bool hasCallRequestFrom(quint32 id) const
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if (!(*itr)->active && (*itr)->id == id)
                return true;
        return false;
    }

    void addCallRequest(CallPeer* peer) { _callPeer.push_back(peer); }
    CallPeer* getCallPeer(quint32 id)
    {
        for (QList<CallPeer*>::iterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->id == id)
                return *itr;
        return NULL;
    }

    CallPeer const* getCallPeer(quint32 id) const
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->id == id)
                return *itr;
        return NULL;
    }

    CallPeer* getCallPeer(QString const& addr, quint16 port = 0)
    {
        for (QList<CallPeer*>::iterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->callIp == addr && ((*itr)->port == port || port == 0))
                return *itr;
        return NULL;
    }

    CallPeer const* getCallPeer(QString const& addr, quint16 port = 0) const
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end(); ++itr)
            if ((*itr)->callIp == addr && ((*itr)->port == port || port == 0))
                return *itr;
        return NULL;
    }

    void clearCallPeers()
    {
        for (QList<CallPeer*>::ConstIterator itr = _callPeer.begin();
             itr != _callPeer.end();)
        {
            CallPeer* peer = *itr;
            ++itr;
            delete peer;
        }
        _callPeer.clear();
    }


    void makeCall(QString const& destEmail, quint32 destId, QString const& destPublicIp, QString const& destPrivateIp);
    void stopCall(QString const& destEmail, quint32 destId, QString const& destPublicIp, QString const& destPrivateIp);

    void clearContacts() { _contactMap.clear(); }
    bool addContact(ContactInfo* info);
    void removeContact(quint32 id);
    ContactInfo* findContact(quint32 id);
    ContactInfo const* findContact(quint32 id) const;
    QMap<quint32, ContactInfo*> const& getContactMap() const { return _contactMap; }


private:
    quint32 _accountId;
    QString _username;
    QString _email;
    QString _publicIp;
    QString _privateIp;

    QList<CallPeer*> _callPeer;
    QMap<quint32, ContactInfo*> _contactMap;
};

#define sClientMgr ClientMgr::instance()

#endif // CLIENTMGR_H
