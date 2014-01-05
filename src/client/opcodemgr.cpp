#include "opcodemgr.h"
#include "mainwindow.h"

OpcodeMgr::OpcodeMgr()
{
}


OpcodeMgr::OpcodeDefinition const* OpcodeMgr::getOpcodeDefinition(quint16 opcode)
{
    static OpcodeDefinition opcodes[] = {
        { SMSG_WELCOME, NULL },
        { CMSG_AUTH, NULL },
        { SMSG_AUTH_RESULT, NULL },
        { SMSG_CONTACT_LOGIN, &MainWindow::handleContactLogin },
        { SMSG_CONTACT_LOGOUT, &MainWindow::handleContactLogout },
        { SMSG_CHAT_TEXT, &MainWindow::handleChatText },
        { SMSG_SEARCH_CONTACT_RESULT, &MainWindow::handleSearchContactResult },
        { SMSG_ADD_CONTACT_REQUEST, &MainWindow::handleAddContactRequest },
        { rMSG_SIP, &MainWindow::handleSipRep},
        { RMSG_SIP, &MainWindow::handleSipRequest},
        { SMSG_ACCOUNT_INFO, &MainWindow::handleAccountInfo },
        { SMSG_JOIN_CHAT_GROUP, &MainWindow::handleJoinChatGroup },
        { SMSG_CHAT_GROUP_ADD_MEMBER, &MainWindow::handleChatGroupAddMember },
        { SMSG_GROUP_CHAT_TEXT, &MainWindow::handleGroupChatText },
        { SMSG_CHAT_GROUP_UPD_MEMBER, &MainWindow::handleChatGroupUpdateMember },
        { SMSG_CONTACT_LIST, &MainWindow::handleContactList },
        { SMSG_CHAT_GROUP_DEL_MEMBER, &MainWindow::handleChatGroupDelMember },
        { MSG_MAX, NULL}
    };

    for (quint32 i = 0; opcodes[i].opcode != MSG_MAX; ++i)
        if (opcodes[i].opcode == opcode)
            return &opcodes[i];
    return NULL;
}
