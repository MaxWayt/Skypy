#include "SessionSocket.h"
#include <boost/bind.hpp>
#include <iostream>
#include "Utils.hpp"
#include "SocketMgr.h"
#include "Session.h"
#include "Skypy.h"
#include "AuthWorker.h"
#include <stdexcept>

SessionSocket::SessionSocket(SocketMgr* mgr) : TcpSocket(mgr->io_service()),
    _sockMgr(mgr), _status(STATUS_UNAUTHED), _session(NULL)
{}

void SessionSocket::onInit()
{
    _status = STATUS_UNAUTHED;

    uint8 buff[] = "WELCOME";
    _send(buff, 8);

    ON_NETWORK_DEBUG(
            std::cout << "Network: SOCKET INIT" << std::endl;
    );

    _registerHeader();
}

void SessionSocket::onClose()
{
    ON_NETWORK_DEBUG(
            std::cout << "Netowkr: ON CLOSE" << std::endl;
    );

    if (getStatus() == STATUS_UNAUTHED)
        _sockMgr->removeNewSock(this);
    else if (_session)
        _session->logout();
}

void SessionSocket::_registerHeader()
{
    boost::asio::async_read(_socket, boost::asio::buffer(_header, Packet::HeaderSize),
            boost::bind(&SessionSocket::_handleHeader, this,
                boost::asio::placeholders::error));
}

void SessionSocket::_handleHeader(boost::system::error_code const& error)
{
    if (error)
    {
        _sockMgr->handleHeaderError(this, std::error_code(/*error.value()*/));
        return;
    }

    uint16_t size = (*((uint16_t const*)&_header[0]));
    Utils::endian::big_to_native<uint16_t>(size);
    uint16_t opcode = (*((uint16_t const*)&_header[2]));
    Utils::endian::big_to_native<uint16_t>(opcode);

    ON_NETWORK_DEBUG(
            std::cout << "Network: HEADER INPUT " << size << " - " << opcode << std::endl;
    );
    if (size > Packet::MaxBodySize)
    {
        _sockMgr->handleInvalidHeaderSize(this, size);
        return;
    }

    boost::asio::async_read(_socket, boost::asio::buffer(_body, size),
            boost::bind(&SessionSocket::_handleBody, this, opcode,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}

void SessionSocket::_handleBody(uint16_t code, boost::system::error_code const& error, std::size_t inputSize)
{
    if (error)
    {
        _sockMgr->handleBodyError(this, std::error_code(/*error.value()*/));
        return;
    }
    ON_NETWORK_DEBUG(
            std::cout << "Network: BODY READED" << std::endl;
    );

    Packet pkt(code, _body, uint16_t(inputSize));
    try
    {
        handlePacketInput(pkt);
    }
    catch (std::exception const& e)
    {
        close();
        delete this;
        return;
    }

    _registerHeader();
}

void SessionSocket::send(Packet const& pkt)
{
    ON_NETWORK_DEBUG(
            std::cout << "Network: SEND OPCODE: " << uint32(pkt.getOpcode()) << std::endl;
    );
    _send(pkt.content(), pkt.size());
}

void SessionSocket::handlePacketInput(Packet& pkt)
{
    ON_NETWORK_DEBUG(
            std::cout << "Network: RECEIV PACKET: " << pkt.getOpcode() << std::endl;
            pkt.dumpHex();
    );
    if (_status == STATUS_UNAUTHED) // Special cases
    {
        switch (pkt.getOpcode())
        {
            case CMSG_AUTH:
                _handleAuthRequest(pkt);
                break;
            default:
                break;
        }
        return;
    }

    if (_session)
        _session->handlePacketInput(pkt);
}

void SessionSocket::_handleAuthRequest(Packet& pkt)
{
    std::string email;
    std::string password;
    pkt >> email >> password;

    ON_NETWORK_DEBUG(
            std::cout << "Network: HANDLE AUTH: " << email << " - " << password << std::endl;
    );

    AuthWorker auth(email, password);

    if (auth.digest().result() == AUTHRESULT_OK)
    {
        _session = new Session(this, email);
        if (!_session->loadFromDb())
        {
            Packet data(SMSG_AUTH_RESULT);
            data << uint8(AUTHRESULT_INTERAL_ERR);
            send(data);
            ON_NETWORK_DEBUG(
                    std::cout << "Network: FAIL TO LOAD USER " << email << " AT LOGIN" << std::endl;
            );
            throw std::runtime_error("Fail to load user");
        }


        _sockMgr->removeNewSock(this);
        _status = STATUS_AUTHED;


        sSkypy->addSession(_session);

        Packet data(SMSG_AUTH_RESULT);
        data << uint8(AUTHRESULT_OK);
        send(data);
        ON_NETWORK_DEBUG(
                std::cout << "Network: SEND LOGIN OK" << std::endl;
        );
    }
    else
    {
        Packet data(SMSG_AUTH_RESULT);
        data << uint8(auth.result());
        send(data);
        ON_NETWORK_DEBUG(
                std::cout << "Network: SEND LOGIN PAS OK, CLOSING" << std::endl;
        );
        throw std::runtime_error("Auth fail");
    }
}
