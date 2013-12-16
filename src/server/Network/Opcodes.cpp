#include "Opcodes.h"

OpcodeMgr::OpcodeDefinition const* OpcodeMgr::getOpcodeDefinition(uint16 opcode)
{

    static OpcodeDefinition opcodes[] = {
        { SMSG_WELCOME, NULL },
        { CMSG_AUTH, NULL },
        { MSG_MAX, NULL }
    };

    for (uint32 i = 0; opcodes[i].opcode != MSG_MAX; ++i)
        if (opcodes[i].opcode == opcode)
            return &opcodes[i];
    return NULL;
}
