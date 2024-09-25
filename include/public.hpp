#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    LOGOUT_MSG,

    REG_MSG,
    REG_MSG_ACK,

    ONE_CHAT_MSG,

    ADD_FRIEND_MSG,
    ADD_FRIEND_ACK,

    CREATE_GROUP_MSG,
    CREATE_GROUP_ACK,

    JOIN_GROUP_MSG,
    JOIN_GROUP_ACK,

    GROUP_CHAT_MSG
};

#endif