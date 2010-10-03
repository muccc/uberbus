#include <avr/wdt.h>
#include <string.h>
#include <stdio.h>

#include "ub.h"
#include "ubpacket.h"
#include "ubslavemgt.h"
#include "ubaddress.h"

#include "settings.h"

enum slavemgtstate {
    DISCOVER,
    IDENTIFY,
    CONNECTED
};

uint8_t ubslavemgt_state;

void ubslavemgt_init(void)
{
    ubslavemgt_state = DISCOVER;
}

uint8_t ubslavemgt_process(struct ubpacket_t * p)
{
    uint8_t * data = p->data;
    struct ubpacket_t * out;

    if(!(p->header.flags & UB_PACKET_MGT))
        return 0;

    switch(data[0]){
        case 'S':
            //d[p->len] = 0;                  //TODO: check bufferlen
            if(ubadr_compareID(data+2)){
                ubadr_setAddress(data[1]);
                ubconfig.configured = 1;
                ubslavemgt_state = IDENTIFY;
            }
        break;
        case 'O':
            ubslavemgt_state = CONNECTED;
        break;
        case 's':
            settings_setid(data+1);
        break;
        case 'r':
            wdt_enable(WDTO_30MS);
            while(1);
        break;
        case 'V':
            out = ubpacket_getSendBuffer();
            out->header.dest = UB_ADDRESS_MASTER;
            out->header.src = ubadr_getAddress();
            out->header.flags = UB_PACKET_MGT;
            sprintf((char *)out->data,"V="__DATE__);
            out->header.len = strlen((char*)out->data);
            ubpacket_send();
        break;
        case 'A':
            ubadr_addMulticast(data[1]);
        break;
        case 'R':
            ubadr_removeMulticast(data[1]);
        break;
        /*case 'g':
            p = packet_getSendBuffer();
            p->dest = UB_ADDRESS_BROADCAST;
            p->flags = 0;
            p->data[0] = 'M';
            p->data[1] = 'N';
            settings_readid(p->data+2);
            p->len = strlen((char*)p->data);
            ubpacket_send();
       break;*/
    }
    return 1;
}

void ubslavemgt_tick(void)
{
    struct ubpacket_t * p;
    static uint16_t time = 0;
    if(!time--){
        time = 1000;
    }

    if(time == 0){
        p = ubpacket_getSendBuffer();
        p->header.src = ubadr_getAddress();
        p->header.flags = UB_PACKET_MGT;

        switch(ubslavemgt_state){
            case DISCOVER:
                p->header.dest = UB_ADDRESS_BROADCAST;
                p->data[0] = MGT_DISCOVER;
                p->data[1] = UB_INTERVAL>>8;
                p->data[2] = UB_INTERVAL&0xFF;
                strcpy((char*)p->data+3,(char*)ubadr_getID());
                p->header.len = strlen((char*)p->data);
                ubpacket_send();
            break;
            case IDENTIFY:
                p->header.dest = UB_ADDRESS_MASTER;
                p->data[0] = MGT_IDENTIFY;
                strcpy((char*)p->data+1,(char*)ubadr_getID());
                p->header.len = strlen((char*)p->data);
                ubpacket_send();
            break;
            case CONNECTED:
                if( ubpacket_free() ){
                    p->header.dest = UB_ADDRESS_MASTER;
                    p->data[0] = MGT_ALIVE;
                    p->header.len = 1;
                    ubpacket_send();
                }
                time = 15000;
           break;
        }
    }
}