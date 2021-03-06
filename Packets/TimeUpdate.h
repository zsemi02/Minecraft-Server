
#ifndef TIMEUPDATE_H
#define TIMEUPDATE_H

#include "Packet.h"


class TimeUpdate: public Packet
{
    public:
        int len = 0;
        long time = 0;
        TimeUpdate(long time): Packet(0x04) {this->time = time;}
        char* build() override;
        void Send(Client* c);
};
#endif