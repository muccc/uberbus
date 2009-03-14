#include <stdint.h>
#include "timer.h"
#include "hal.h"
#include "bus.h"
#include "uart.h"
#include "random.h"
#include "bridge.h"
#include "frame.h"
#include "serial_handler.h"

void bridge_init(uint8_t addr)
{
}
volatile uint8_t flag;
void bridge_mainloop(void)
{
    struct frame * f;
    struct frame s;
    s.len = 6;
    strcpy(s.data,"FNORD2");
	while (1){
        cli();
        f = bus_frame;
        sei();
        if( f->isnew == 1){
            f->data[f->len]=0;
            DEBUG("new frame len=%u data=%s",f->len, f->data);
            f->isnew = 0;
        }
        if(flag){
            //DEBUG("TICK");
            flag = 0;
            DEBUG("S");
            bus_send(&s,1);
            //DEBUG("D");
        }
        wdt_reset();
	}
}

void bridge_tick(void)
{
    static uint16_t i = 1;
    if(--i == 0){
        flag = 1;
        i = 500;
    }
}
