#ifndef OPCODEMGR_H
#define OPCODEMGR_H

#include "packet.hpp"

enum Opcodes
{
    SMSG_WELCOME        = 1,
    CMSG_AUTH           = 2,
    SMSG_AUTH_RESULT    = 3,
    rMSG_SIP            = 4,
    RMSG_SIP            = 5,
    SMSG_CONTACT_LOGIN  = 6,
    SMSG_CONTACT_LOGOUT = 7,
    CMSG_CHAT_TEXT      = 8,
    SMSG_CHAT_TEXT      = 9,
    MSG_MAX
};

class MainWindow;

class OpcodeMgr
{
public:
    OpcodeMgr();

    struct OpcodeDefinition
    {
        quint16 opcode;
        void (MainWindow::*func)(Packet&);
    };

    static OpcodeDefinition const* getOpcodeDefinition(quint16 opcode);
};

#endif // OPCODEMGR_H
