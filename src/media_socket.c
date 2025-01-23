#include "../headers/media_socket.h"

pj_status_t media_socket_create(pj_pool_t *pool, 
                                pjmedia_endpt *endpt,  
                                const pj_uint16_t af,
                                const int rtp_port, 
                                struct media_socket_t **socket) 
{
    pj_status_t status;
     
    (*socket) = (struct media_socket_t*) pj_pool_alloc(pool, sizeof(**socket)); 
    if (!socket) {
        return FAILURE; 
    }   
    
    (*socket)->occupied = PJ_FALSE;

    status = pjmedia_transport_udp_create3(endpt, af, NULL, NULL, 
                                           rtp_port, 0, 
                                           &(*socket)->med_transport);
    if (status != PJ_SUCCESS) 
    {
        return status;
    }

    pjmedia_transport_info_init(&(*socket)->med_tpinfo);
    pjmedia_transport_get_info((*socket)->med_transport, &(*socket)->med_tpinfo);

    pj_memcpy(&(*socket)->sock_info, &(*socket)->med_tpinfo.sock_info,
              sizeof(pjmedia_sock_info));

    return PJ_SUCCESS;
}

void media_socket_free(struct media_socket_t *socket) {
    if (socket->med_transport)
        pjmedia_transport_close(socket->med_transport);
}
