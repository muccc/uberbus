#include <stdint.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "packet.h"
#include "ubpacket.h"
#include "message.h"
#include "serial.h"
#include "busmgt.h"
#include "nodes.h"

//static void packet_inpacket(struct ubpacket* p);
static gboolean packet_inpacket(gpointer);
//GHashTable* packet_callbacks;

GAsyncQueue * packet_status;
GAsyncQueue * packet_out;
struct node *nextnode;

gpointer packet_readerThread(gpointer data)
{
    data = NULL;
    struct message in;

    printf("started packet_readerThread()\n");
    while( 1 ){
        serial_readMessage(&in);
        if( in.len > 0){
            switch( in.data[0] ){
                case PACKET_PACKET:
                    printf("new packet\n");
                    if( sizeof(struct ubpacket) >= in.len ){
                        struct packetstream *ps =
                                g_new(struct packetstream,1);
                        memcpy(&ps->p,in.data+1,in.len-1);
                        struct node *n = nodes_getNodeByBusAdr(ps->p.src);
                        ps->n = n;
                        ps->type = PACKET_PACKET;
                        if( n ){
                            ps->callback = n->currentcallback;
                            ps->data = n->currentdata;
                        }else{
                            ps->callback = ps->data = NULL;
                        }
                        //g_async_queue_push(packet_in,ps);
                        g_idle_add(packet_inpacket, ps);
                    }else{
                        //TODO: log this error
                    }
                break;
                case PACKET_DONE:
                    printf("new message: ");
                    printf("%c: packet done\n", in.data[0]);
                    if( nextnode ){
                        struct packetstream *ps =
                            g_new(struct packetstream,1);
                        ps->callback = nextnode->currentcallback =
                                        nextnode->nextcallback;
                        ps->data = nextnode->currentdata =
                                        nextnode->nextdata;
                        ps->type = PACKET_DONE;
                        g_idle_add(packet_inpacket, ps);
                    }
                    g_async_queue_push(packet_status,
                                (gpointer)PACKET_DONE);
                break;
                case PACKET_ABORT:
                    printf("new message: ");
                    printf("packet aborted\n");

                    if( nextnode ){
                        struct packetstream *ps =
                            g_new(struct packetstream,1);
                        ps->type = PACKET_ABORT;
                        ps->data = nextnode->nextdata;
                        ps->callback = nextnode->nextcallback;
                        g_idle_add(packet_inpacket, ps);
                        nextnode->currentcallback =
                            nextnode->nextcallback = NULL;
                    }

                    g_async_queue_push(packet_status,
                                (gpointer)PACKET_ABORT);
                break;
                case 'D':
                    //printf("debug\n");
                break;
            }
        }
    }
}

gpointer packet_writerThread(gpointer data)
{
    data = NULL;
    while( 1 ){
        struct messagestream * msg = (struct messagestream *)
                                g_async_queue_pop(packet_out);
        g_assert(msg != NULL);
        
        struct ubpacket * p = (struct ubpacket *)msg->msg.data;
        debug_packet("packet_writerThread",p);
        //this becomes the current stream after receiving the ack
        //the reader thread will read this after getting the ack
        //produced by writeMessage()
        //so no mutex seems to be needed
        nextnode = msg->n;
        if( msg->n != NULL ){
            msg->n->nextcallback = msg->callback;
            msg->n->nextdata = msg->data;
        }
        serial_writeMessage(&msg->msg);
        g_free(msg);
        
        GTimeVal timeout;
        g_get_current_time(&timeout);
        g_time_val_add(&timeout,2 * 1000 * 1000);

        gpointer status = g_async_queue_timed_pop(packet_status,&timeout);
        //TODO: this assert is bad as this could happen
        g_assert(status != NULL);

        if( status == (gpointer)PACKET_ABORT ){
            //TODO: add log
            printf("PACKET WAS ABORTED\n");
        }else if( status == (gpointer)PACKET_DONE ){
            //printf("packet done\n");
        }
    }
}

/*static void printkey(gpointer key, gpointer value, gpointer data)
{
    value = NULL;
    data = NULL;
    printf(key);
}*/

static gboolean packet_inpacket(gpointer data)
{
    //data = NULL;
    /*void(*cb)(struct ubpacket*);
    gchar buf[2];
    buf[0] = p->data[0];
    buf[1] = 0;
    printf("registred:\n");
    g_hash_table_foreach(packet_callbacks,printkey,NULL);
    printf("end\n");
    cb = g_hash_table_lookup(packet_callbacks, buf);
    if( cb ){
        cb(p);
    }else{
        printf("There is no handler registerd for packet type %s\n",buf);
    }*/
    g_assert(data != NULL);
    struct packetstream *ps = (struct packetstream *)data;
    if( ps->type == PACKET_PACKET )
        debug_packet("packet_inpacket",&ps->p);

    //TODO: Check if the packet was sent unsolicited
    if( ps->callback != NULL &&
                    (ps->type != PACKET_PACKET ||
                            !(ps->p.flags & UB_PACKET_UNSOLICITED)) ){
        printf("forwarding packet\n");
        ps->callback(ps);
    }else if( ps->type == PACKET_PACKET && ps->p.flags & UB_PACKET_MGT ){
        printf("for bus mgt\n");
        busmgt_inpacket(&ps->p);
    }else if( ps->type == PACKET_PACKET ){
        printf("unsolicited data. not yet processed.\n");
    }else if( ps->type != PACKET_PACKET ){
        printf("unsolicited status information. not processed.\n");
    }else{
        printf("should not happen\n");
        while(1);
    }

    g_free(ps);
    return FALSE;
}

void packet_init(void)
{
    packet_status = g_async_queue_new();
    packet_out = g_async_queue_new();
    g_thread_create(packet_readerThread,NULL,FALSE,NULL);
    g_thread_create(packet_writerThread,NULL,FALSE,NULL);
}

/*void packet_addCallback(gchar key, void(*cb)(struct ubpacket*))
{
    gchar buf[2];
    buf[0] = key;
    buf[1] = 0;

    g_hash_table_insert(packet_callbacks, g_strdup(buf), cb);

    printf("Added callback for message type %c to 0x%x\n",key,(unsigned int)cb);
}*/

void packet_streamPacket(struct node * n, struct ubpacket *p,
                            UBSTREAM_CALLBACK callback, gpointer data)
{
    struct messagestream * outmsg = g_new(struct messagestream,1);
    p->src = 1;
    p->flags &= UB_PACKET_MGT | UB_PACKET_NOACK;

    printf("sending packet with dest=%u src=%u flags=%u len=%u\n",
                p->dest, p->src, p->flags,p->len);
    
    outmsg->msg.len = p->len+UB_PACKET_HEADER;
    //g_assert(p->len + UB_PACKET_HEADER < );
    memcpy(outmsg->msg.data, p, p->len + UB_PACKET_HEADER);
    outmsg->callback = callback;
    outmsg->data = data;
    outmsg->n = n;
    g_async_queue_push(packet_out,outmsg);
}

void packet_outpacket(struct ubpacket* p)
{
    packet_streamPacket(NULL, p, NULL, NULL);
}
