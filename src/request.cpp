//------------------------------------------------------------------------------
//
//
//
//
//
//------------------------------------------------------------------------------
/*!
 * \file
 * \brief Modbus request data structures
 * \copyright maisvendoo
 * \author Dmitry Pritykin
 * \date 23/11/2017
 */

#include    "request.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
quint16 calcCRC16(char *buff, quint8 size)
{
    quint32 tmp, tmp2, flag;

    tmp = 0xFFFF;

    for (quint8 i = 0; i < size; i++)
    {
        tmp = tmp ^ buff[i];

        for (quint8 j = 1; j <= 8; j++)
        {
            flag = tmp & 0x0001;
            tmp >>= 1;

            if (flag)
                tmp ^= 0xA001;
        }
    }

    tmp2 = tmp >> 8;
    tmp = (tmp << 8) | tmp2;
    tmp &= 0xFFFF;

    return tmp;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
quint8 hiByte(quint16 value)
{
    return static_cast<quint8>(value >> 8);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
quint8 loByte(quint16 value)
{
    return static_cast<quint8>(value);
}
