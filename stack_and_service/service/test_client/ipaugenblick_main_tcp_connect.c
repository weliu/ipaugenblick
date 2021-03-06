#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ipaugenblick_api.h>
#include <string.h>

#define USE_TX 1
#define USE_RX 1

int main(int argc,char **argv)
{
    void *txbuff,*rxbuff,*pdesc;
    int sock,newsock,len;
    char *p;
    int size = 0,ringset_idx;
    int sockets_connected = 0;
    int selector;
    int ready_socket;
    int i,tx_space;
    int max_total_length = 0;
    unsigned short mask;
    unsigned long received_count = 0;

    if(ipaugenblick_app_init(argc,argv,"tcp_connect") != 0) {
        printf("cannot initialize memory\n");
        return 0;
    } 
    printf("memory initialized\n");
    selector = ipaugenblick_open_select();
    if(selector != -1) {
        printf("selector opened %d\n",selector);
    }

    if((sock = ipaugenblick_open_socket(AF_INET,SOCK_STREAM,selector)) < 0) {
        printf("cannot open tcp client socket\n");
        return 0;
    }
    printf("opened socket %d\n",sock);
    int bufsize = 1024*1024*1000;
    ipaugenblick_setsockopt(sock, SOL_SOCKET,SO_SNDBUFFORCE,(char *)&bufsize,sizeof(bufsize));
    ipaugenblick_setsockopt(sock, SOL_SOCKET,SO_RCVBUFFORCE,(char *)&bufsize,sizeof(bufsize));
    ipaugenblick_v4_connect_bind_socket(sock,inet_addr("192.168.150.62"),htons(7777),1);
 
    int first_time = 1;
    while(1) {  
        ready_socket = ipaugenblick_select(selector,&mask,NULL);
        if(ready_socket == -1) {
            continue;
        }
        if(first_time) {
		ipaugenblick_socket_kick(ready_socket);
		first_time = 0;
	}
        
        if(mask & /*SOCKET_READABLE_BIT*/0x1) {
#if USE_RX
	    int first_seg_len = 0;
	    int len = 0;
            while(ipaugenblick_receive(ready_socket,&rxbuff,&len,&first_seg_len,&pdesc) == 0) {
                received_count++;
		void *porigdesc = pdesc;

#if 0
                while(rxbuff) { 
                    if(len > 0) {
                        /* do something */
                        /* don't release buf, release rxbuff */
                    }
		    rxbuff = ipaugenblick_get_next_buffer_segment(&pdesc,&len);
                }

                if(!(received_count%10000000)) {
                    printf("received %u max_total_len %u\n",received_count,max_total_length);
                }
#endif
                ipaugenblick_release_rx_buffer(porigdesc,ready_socket);
            }
#endif
        }
#if USE_TX
        if(mask & /*SOCKET_WRITABLE_BIT*/0x2) {	
            tx_space = ipaugenblick_get_socket_tx_space(ready_socket);
#if 0 
            for(i = 0;i < tx_space;i++) {
                txbuff = ipaugenblick_get_buffer(1448,ready_socket,&pdesc);
                if(!txbuff) {
                    break;
                }
                if(ipaugenblick_send(ready_socket,pdesc,0,1448)) { 
                    ipaugenblick_release_tx_buffer(pdesc);
                    break;
                }
            } 
#else
            if(tx_space == 0)
                continue;
            struct data_and_descriptor bulk_bufs[tx_space];
            int offsets[tx_space];
            int lengths[tx_space];
            if(!ipaugenblick_get_buffers_bulk(1448,ready_socket,tx_space,bulk_bufs)) {
                    for(i = 0;i < tx_space;i++) {
                        offsets[i] = 0;
                        lengths[i] = 1448;
                    }
                    if(ipaugenblick_send_bulk(ready_socket,bulk_bufs,offsets,lengths,tx_space)) {
                        for(i = 0;i < tx_space;i++)
                                ipaugenblick_release_tx_buffer(bulk_bufs[i].pdesc);
                        printf("%s %d\n",__FILE__,__LINE__);
                    }
                    else {
//                        printf("%s %d %d\n",__FILE__,__LINE__,tx_space);
//                        sent = 1;
                    }
            }
#endif
            ipaugenblick_socket_kick(ready_socket);
        }  
//        ipaugenblick_socket_kick(ready_socket);
#endif
    }
    return 0;
}
