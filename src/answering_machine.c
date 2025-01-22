#include "../headers/answering_machine.h"

/* Prototypes */
static pj_status_t answering_machine_ua_module_init(pjsip_module *module);

static pj_status_t answering_machine_logger_module_init(pjsip_module *module);

static pj_status_t answering_machine_global_endpt_init(void);

static pj_status_t answering_machine_transport_init(void);

static pj_status_t answering_machine_invite_module_init(void);

static pj_status_t answering_machine_media_endpt_init(void);

static pj_status_t answering_machine_media_transport_create(void);

static pj_status_t answering_machine_add_call(struct call_t *call);

static pj_status_t answering_machine_find_socket(struct media_socket_t **socket);

static pj_status_t answering_machine_find_call(const pj_str_t *dlg_id, struct call_t **call);

static pj_status_t answering_machine_delete_call(const pj_str_t *dlg_id);

static void answering_machine_free(struct answering_machine_t *machine_ptr);

/* Notification on incoming messages */
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata);

/* Notification on outgoing messages */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata);

/* Callback to be called when SDP negotiation is done in the call: */
static void call_on_media_update(pjsip_inv_session *inv, pj_status_t status);

/* Callback to be called when invite session's state has changed: */
static void call_on_state_changed(pjsip_inv_session *inv, pjsip_event *e);

/* Callback to be called when dialog has forked: */
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e);

static void on_ringing_timer_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);

static void on_media_state_timer_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);

/* Callback to be called to handle incoming requests outside dialogs: */
static pj_bool_t on_rx_request(pjsip_rx_data *rdata);

/* Global variables */
struct answering_machine_t *machine;

pj_caching_pool cp;

pj_status_t answering_machine_create(pj_pool_t **pool)
{
    pj_status_t status;
    int a = PJ_TRUE;
    /* Init PJLIB */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    pj_log_set_level(5);

    /* Init PJLIB-UTIL */
    status = pjlib_util_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Create pool factory and machine pool */
    pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

    *pool = pj_pool_create(&cp.factory, "answering_machine_pool", POOL_SIZE, POOL_INC, NULL);

    /* Init machine */
    machine = (struct answering_machine_t *)pj_pool_alloc(*pool, sizeof(*machine));
    machine->calls = (struct call_t **)pj_pool_alloc(*pool, MAX_CALLS * sizeof(*machine->calls));
    machine->med_sockets =
        (struct media_socket_t **)pj_pool_alloc(*pool, MAX_MEDIA_CNT * sizeof(*machine->med_sockets));

    machine->cp = &cp;
    machine->pool = *pool;
    machine->calls_count = 0;
    machine->calls_capacity = MAX_CALLS;

    answering_machine_global_endpt_init();

    answering_machine_transport_init();

    /* Init modules */
    status = pjsip_tsx_layer_init_module(machine->g_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = pjsip_ua_init_module(machine->g_endpt, NULL);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    answering_machine_invite_module_init();

    answering_machine_ua_module_init(&machine->mod_simpleua);
    answering_machine_logger_module_init(&machine->msg_logger);

    status = pjsip_100rel_init_module(machine->g_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Register modules */
    status = pjsip_endpt_register_module(machine->g_endpt, &machine->mod_simpleua);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    status = pjsip_endpt_register_module(machine->g_endpt, &machine->msg_logger);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    answering_machine_media_endpt_init();

    /* Create event manager */
    status = pjmedia_event_mgr_create(*pool, 0, NULL);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    answering_machine_media_transport_create();

    status =
        pjmedia_conf_create(machine->pool, MAX_MEDIA_CNT, CLOCK_RATE, NCHANNELS, NSAMPLES, NBITS, PJMEDIA_CONF_NO_DEVICE, &machine->conf);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    machine->table = pj_hash_create(machine->pool, 1000);

    return status;
}

void answering_machine_calls_recv(void)
{
    pjmedia_port *zero_port = pjmedia_conf_get_master_port(machine->conf);
    pjmedia_frame frame;
    frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
    frame.size = NSAMPLES * NBITS / 8;

    PJ_LOG(3, (THIS_FILE, "Ready to accept incoming calls..."));
    while (1)
    {
        /*char option[10];

        puts("Press 'q' to quit");
        if (fgets(option, sizeof(option), stdin) == NULL) {
            puts("EOF while reading stdin, will quit now...");
        }

        if (option[0] == 'q') {
            break;
        }*/
        pj_time_val timeout = {0, 10};
        pjmedia_port_get_frame(zero_port, &frame);
        pjsip_endpt_handle_events(machine->g_endpt, &timeout);
    }

    answering_machine_free(machine);
}

void answering_machine_signal_add(pjmedia_port *signal, char *username)
{
    unsigned int p_slot;

    pjmedia_conf_add_port(machine->conf, machine->pool, signal, NULL, &p_slot);
    pj_hash_set(machine->pool, machine->table, username, PJ_HASH_KEY_STRING, 0, &p_slot);
}

static pj_status_t answering_machine_ua_module_init(pjsip_module *module)
{
    if (module == NULL)
    {
        app_perror(THIS_FILE, "Logger module is NULL", 1);
    }

    module->prev = NULL;
    module->next = NULL;
    module->name = pj_str("mod-simpleua");
    module->id = -1;
    module->priority = PJSIP_MOD_PRIORITY_APPLICATION;

    module->load = NULL;
    module->start = NULL;
    module->stop = NULL;
    module->unload = NULL;

    module->on_rx_request = &on_rx_request;
    module->on_rx_response = NULL;
    module->on_tx_request = NULL;
    module->on_tx_response = NULL;
    module->on_tsx_state = NULL;

    return PJ_SUCCESS;
}

static pj_status_t answering_machine_logger_module_init(pjsip_module *module)
{
    if (module == NULL)
    {
        app_perror(THIS_FILE, "Logger module is NULL", 1);
    }

    module->prev = NULL;
    module->next = NULL;
    module->name = pj_str("mod-msg-log");
    module->id = -1;
    module->priority = PJSIP_MOD_PRIORITY_TRANSPORT_LAYER - 1;

    module->load = NULL;
    module->start = NULL;
    module->stop = NULL;
    module->unload = NULL;

    module->on_rx_request = &logging_on_rx_msg;
    module->on_rx_response = &logging_on_rx_msg;
    module->on_tx_request = &logging_on_tx_msg;
    module->on_tx_response = &logging_on_tx_msg;
    module->on_tsx_state = NULL;

    return PJ_SUCCESS;
}

static pj_status_t answering_machine_global_endpt_init(void)
{
    pj_status_t status;
    const pj_str_t *hostname;
    const char *endpt_name;

    /* Endpoint MUST be assigned a globally unique name.
     * The name will be used as the hostname in Warning header.
     */

    /* For this implementation, we'll use hostname for simplicity */
    hostname = pj_gethostname();
    endpt_name = hostname->ptr;

    /* Create the endpoint: */
    status = pjsip_endpt_create(&machine->cp->factory, endpt_name, &machine->g_endpt);

    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    return status;
}

static pj_status_t answering_machine_transport_init(void)
{
    pj_status_t status;
    pj_sockaddr addr;
    int af = AF;

    pj_sockaddr_init(af, &addr, NULL, (pj_uint16_t)SIP_PORT);

    if (af == pj_AF_INET())
    {
        status = pjsip_udp_transport_start(machine->g_endpt, &addr.ipv4, NULL, 1, NULL);
    }
    else if (af == pj_AF_INET6())
    {
        status = pjsip_udp_transport_start6(machine->g_endpt, &addr.ipv6, NULL, 1, NULL);
    }
    else
    {
        status = PJ_EAFNOTSUP;
    }

    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to start UDP transport", status);
        return 1;
    }

    return status;
}

static pj_status_t answering_machine_invite_module_init(void)
{
    pjsip_inv_callback inv_cb;
    pj_status_t status;

    /* Init the callback for INVITE session: */
    pj_bzero(&inv_cb, sizeof(inv_cb));
    inv_cb.on_state_changed = &call_on_state_changed;
    inv_cb.on_new_session = &call_on_forked;
    inv_cb.on_media_update = &call_on_media_update;

    /* Initialize invite session module:  */
    status = pjsip_inv_usage_init(machine->g_endpt, &inv_cb);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    return status;
}

static pj_status_t answering_machine_media_endpt_init(void)
{
    pj_status_t status;

#if PJ_HAS_THREADS
    status = pjmedia_endpt_create(&machine->cp->factory, NULL, 1, &machine->g_med_endpt);
#else
    status = pjmedia_endpt_create(
        &machine->cp->factory, pjsip_endpt_get_ioqueue(machine->g_endpt), 0, &machine->g_med_endpt);
#endif

    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);

    /* Create pool. */
    machine->media_pool = pjmedia_endpt_create_pool(machine->g_med_endpt, "Media pool", 512, 512);

    /*
     * Add PCMA/PCMU codec to the media endpoint.
     */
#if defined(PJMEDIA_HAS_G711_CODEC) && PJMEDIA_HAS_G711_CODEC != 0
    status = pjmedia_codec_g711_init(machine->g_med_endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, 1);
#endif

    return status;
}

static pj_status_t answering_machine_media_transport_create(void)
{
    int i;
    pj_status_t status;

    for (i = 0; i < PJ_ARRAY_SIZE(machine->med_sockets); ++i)
    {
        status =
            media_socket_create(machine->pool, machine->g_med_endpt, AF, RTP_PORT + i * 2, &machine->med_sockets[i]);
        machine->g_sock_info[i] = machine->med_sockets[i]->sock_info;
    }

    return status;
}

static pj_status_t answering_machine_add_call(struct call_t *call)
{
    int i = machine->calls_count;

    if (machine->calls_count == machine->calls_capacity)
    {
        return -1;
    }

    machine->calls[i] = call;
    machine->calls_count++;

    return PJ_SUCCESS;
}

static pj_status_t answering_machine_find_call(const pj_str_t *dlg_id, struct call_t **call)
{
    for (int i = 0; i < machine->calls_count; i++)
    {
        if (pj_strcmp(&machine->calls[i]->call_id, dlg_id) == 0)
        {
            *call = machine->calls[i];
            return PJ_SUCCESS;
        }
    }

    return -1;
}

static pj_status_t answering_machine_delete_call(const pj_str_t *dlg_id)
{
    int i;

    /* Find call by dlg id */
    for (i = 0; i < machine->calls_count; i++)
    {
        if (pj_strcmp(&machine->calls[i]->call_id, dlg_id) == 0)
        {
            free_call(machine->calls[i]);
        }
    }

    /* Shift calls by 1 to left */
    for (i = i; i < machine->calls_count; i++)
    {
        machine->calls[i] = machine->calls[i + 1];
    }
    machine->calls_count--;

    return PJ_SUCCESS;
}

static pj_status_t answering_machine_find_socket(struct media_socket_t **socket)
{
    for (int i = 0; i < MAX_MEDIA_CNT; i++)
    {
        if (machine->med_sockets[i]->occupied == PJ_FALSE)
        {
            *socket = machine->med_sockets[i];
            return PJ_SUCCESS;
        }
    }

    return -1;
}

static void answering_machine_free(struct answering_machine_t *machine)
{
    int i;

    /* Destroy media transports */
    for (i = 0; i < MAX_MEDIA_CNT; ++i)
    {
        media_socket_free(machine->med_sockets[i]);
    }

    /* Destroy event manager */
    pjmedia_event_mgr_destroy(NULL);

    /* Deinit pjmedia endpoint */
    if (machine->g_med_endpt)
        pjmedia_endpt_destroy(machine->g_med_endpt);

    /* Deinit pjsip endpoint */
    if (machine->g_endpt)
        pjsip_endpt_destroy(machine->g_endpt);

    /* Release media pool */
    if (machine->media_pool)
        pj_pool_release(machine->media_pool);

    /* Release answering_machine pool */
    if (machine->pool)
        pj_pool_release(machine->pool);
}

/* Notification on incoming messages */
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata)
{
    PJ_LOG(4,
           (THIS_FILE,
            "RX %d bytes %s from %s %s:%d:\n"
            "%.*s\n"
            "--end msg--",
            rdata->msg_info.len,
            pjsip_rx_data_get_info(rdata),
            rdata->tp_info.transport->type_name,
            rdata->pkt_info.src_name,
            rdata->pkt_info.src_port,
            (int)rdata->msg_info.len,
            rdata->msg_info.msg_buf));

    /* Always return false, otherwise messages will not get processed! */
    return PJ_FALSE;
}

/* Notification on outgoing messages */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata)
{

    /* Important note:
     *  tp_info field is only valid after outgoing messages has passed
     *  transport layer. So don't try to access tp_info when the module
     *  has lower priority than transport layer.
     */

    PJ_LOG(4,
           (THIS_FILE,
            "TX %ld bytes %s to %s %s:%d:\n"
            "%.*s\n"
            "--end msg--",
            (tdata->buf.cur - tdata->buf.start),
            pjsip_tx_data_get_info(tdata),
            tdata->tp_info.transport->type_name,
            tdata->tp_info.dst_name,
            tdata->tp_info.dst_port,
            (int)(tdata->buf.cur - tdata->buf.start),
            tdata->buf.start));

    /* Always return success, otherwise message will not get sent! */
    return PJ_SUCCESS;
}

/*
 * Callback when SDP negotiation has completed.
 * We are interested with this callback because we want to start media
 * as soon as SDP negotiation is completed.
 */
static void call_on_media_update(pjsip_inv_session *inv, pj_status_t status)
{
    pjmedia_stream_info stream_info;
    const pjmedia_sdp_session *local_sdp;
    const pjmedia_sdp_session *remote_sdp;
    unsigned int conf_slot;
    unsigned int player_slot;
    pjmedia_port *media_port;
    struct call_t *call;

    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "SDP negotiation has failed", status);
        return;
    }

    /* Get local and remote SDP.
     * We need both SDPs to create a media session.
     */
    status = pjmedia_sdp_neg_get_active_local(inv->neg, &local_sdp);

    status = pjmedia_sdp_neg_get_active_remote(inv->neg, &remote_sdp);

    /* Create stream info based on the media audio SDP. */
    status = pjmedia_stream_info_from_sdp(&stream_info, inv->dlg->pool, machine->g_med_endpt, local_sdp, remote_sdp, 0);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to create audio stream info", status);
        return;
    }

    status = answering_machine_find_call(&inv->dlg->call_id->id, &call);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to find call", status);
        return;
    }

    status = answering_machine_find_socket(&call->socket);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to find unoccupied socket", status);
        return;
    }

    /* Create new audio media stream, passing the stream info, and also the
     * media socket that we created earlier.
     */
    status = pjmedia_stream_create(
        machine->g_med_endpt, inv->dlg->pool, &stream_info, call->socket->med_transport, NULL, &call->med_stream);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to create audio stream", status);
        return;
    }

    /* Start the audio stream */
    status = pjmedia_stream_start(call->med_stream);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to start audio stream", status);
        return;
    }

    /* Start the UDP media transport */
    /*status = pjmedia_transport_media_start(call->socket->med_transport, 0, 0, 0, 0);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to start UDP media transport", status);
        return;
    }*/

    /* Get the media port interface of the audio stream */
    status = pjmedia_stream_get_port(call->med_stream, &media_port);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to create media port interface of the audio stream", status);
        return;
    }

    /* Add media port to conf bridge */
    pjmedia_conf_add_port(machine->conf, machine->pool, media_port, NULL, &conf_slot);

    /* TODO: Find conf bridge slot for username */

    /* TODO: Link call port to player port in conf bridge */
    pjmedia_conf_connect_port(machine->conf, *call->player_port, conf_slot, 0);

    pjsip_endpt_schedule_timer(machine->g_endpt, call->media_session_timer, &call->media_session_time);
}

/*
 * Callback when INVITE session state has changed.
 * This callback is registered when the invite session module is initialized.
 * We mostly want to know when the invite session has been disconnected,
 * so that we can quit the application.
 */
static void call_on_state_changed(pjsip_inv_session *inv, pjsip_event *e)
{
    struct call_t *call;
    PJ_UNUSED_ARG(e);

    if (inv->state == PJSIP_INV_STATE_DISCONNECTED)
    {
        PJ_LOG(3,
               (THIS_FILE, "Call DISCONNECTED [reason=%d (%s)]", inv->cause, pjsip_get_status_text(inv->cause)->ptr));
        answering_machine_find_call(&inv->dlg->call_id->id, &call);

        pjsip_endpt_cancel_timer(machine->g_endpt, call->ringing_timer);
        pjsip_endpt_cancel_timer(machine->g_endpt, call->media_session_timer);

        answering_machine_delete_call(&inv->dlg->call_id->id);
    }
    else
    {
        PJ_LOG(3, (THIS_FILE, "Call state changed to %s", pjsip_inv_state_name(inv->state)));
    }
}

/* This callback is called when dialog has forked. */
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e)
{
    /* To be done... */
    PJ_UNUSED_ARG(inv);
    PJ_UNUSED_ARG(e);
}

static void on_ringing_timer_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)
{
    pj_status_t status;
    pjsip_tx_data *tdata;
    struct call_t *call = entry->user_data;

    PJ_UNUSED_ARG(timer_heap);

    status = pjsip_inv_answer(call->inv,
                              200,
                              NULL, /* st_code and st_text */
                              NULL, /* SDP already specified */
                              &tdata);

    /*
     * Send the 200 response.
     */
    status = pjsip_inv_send_msg(call->inv, tdata);
}

static void on_media_state_timer_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)
{
    pj_status_t status;
    pjsip_tx_data *tdata;
    struct call_t *call = entry->user_data;

    PJ_UNUSED_ARG(timer_heap);

    status = pjsip_inv_end_session(call->inv,
                                   403,  /* st_code and st_text */
                                   NULL, /* SDP already specified */
                                   &tdata);

    status = pjsip_inv_send_msg(call->inv, tdata);
}

/*
 * Callback when incoming requests outside any transactions and any
 * dialogs are received. We're only interested to hande incoming INVITE
 * request, and we'll reject any other requests with 500 response.
 */
static pj_bool_t on_rx_request(pjsip_rx_data *rdata)
{
    pj_sockaddr hostaddr;
    char temp[80], hostip[PJ_INET6_ADDRSTRLEN];
    pj_str_t local_uri;
    pjsip_dialog *dlg;
    pjmedia_sdp_session *local_sdp;
    pjsip_user_agent *ua;
    pjsip_tx_data *tdata;
    pjsip_sip_uri *uri;
    unsigned options = 0;
    unsigned int *player_port;
    pj_status_t status;
    struct call_t *call;

    /*
     * Respond (statelessly) any non-INVITE requests with 500
     */
    if (rdata->msg_info.msg->line.req.method.id != PJSIP_INVITE_METHOD)
    {

        if (rdata->msg_info.msg->line.req.method.id != PJSIP_ACK_METHOD)
        {
            pj_str_t reason = pj_str("Simple UA unable to handle "
                                     "this request");

            pjsip_endpt_respond_stateless(machine->g_endpt, rdata, 500, &reason, NULL, NULL);
        }
        return PJ_TRUE;
    }

    /* Verify that we can handle the request. */
    status = pjsip_inv_verify_request(rdata, &options, NULL, NULL, machine->g_endpt, NULL);
    if (status != PJ_SUCCESS)
    {

        pj_str_t reason = pj_str("Sorry Simple UA can not handle this INVITE");

        pjsip_endpt_respond_stateless(machine->g_endpt, rdata, 500, &reason, NULL, NULL);
        return PJ_TRUE;
    }

    /* Verify username */
    uri = (pjsip_sip_uri *)pjsip_uri_get_uri(rdata->msg_info.to->uri);
    player_port = pj_hash_get(machine->table, uri->user.ptr, uri->user.slen, 0);
    if (player_port == NULL)
    {
        pj_str_t reason = pj_str("Can't find username");

        pjsip_endpt_respond_stateless(machine->g_endpt, rdata, 403, &reason, NULL, NULL);
        return PJ_TRUE;
    }

    /*
     * Generate Contact URI
     */
    if (pj_gethostip(AF, &hostaddr) != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Unable to retrieve local host IP", status);
        return PJ_TRUE;
    }
    pj_sockaddr_print(&hostaddr, hostip, sizeof(hostip), 2);

    pj_ansi_snprintf(temp, sizeof(temp), "<sip:simpleuas@%s:%d>", hostip, SIP_PORT);
    local_uri = pj_str(temp);

    /*
     * Create UAS dialog.
     */
    ua = pjsip_ua_instance();
    status = pjsip_dlg_create_uas_and_inc_lock(ua,
                                               rdata,
                                               &local_uri, /* contact */
                                               &dlg);
    if (status != PJ_SUCCESS)
    {
        pjsip_endpt_respond_stateless(machine->g_endpt, rdata, 500, NULL, NULL, NULL);
        return PJ_TRUE;
    }

    status = create_call(dlg->pool, dlg->call_id->id, &call);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error in call creation", status);
    }

    call->player_port = player_port;

    status = answering_machine_add_call(call);
    if (status != PJ_SUCCESS)
    {
        app_perror(THIS_FILE, "Error in adding call to array", status);
    }

    /*
     * Get media capability from media endpoint:
     */
    status = pjmedia_endpt_create_sdp(
        machine->g_med_endpt, rdata->tp_info.pool, MAX_MEDIA_CNT, machine->g_sock_info, &local_sdp);
    pj_assert(status == PJ_SUCCESS);
    if (status != PJ_SUCCESS)
    {
        pjsip_dlg_dec_lock(dlg);
        return PJ_TRUE;
    }

    /*
     * Create invite session, and pass both the UAS dialog and the SDP
     * capability to the session.
     */
    status = pjsip_inv_create_uas(dlg, rdata, local_sdp, 0, &call->inv);
    pj_assert(status == PJ_SUCCESS);
    if (status != PJ_SUCCESS)
    {
        pjsip_dlg_dec_lock(dlg);
        return PJ_TRUE;
    }

    /*
     * Invite session has been created, decrement & release dialog lock.
     */
    pjsip_dlg_dec_lock(dlg);

    /* Create initial 180 response */
    status = pjsip_inv_initial_answer(call->inv, rdata, 180, NULL, NULL, &tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_TRUE);

    /* Send the 180 response. */
    status = pjsip_inv_send_msg(call->inv, tdata);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, PJ_TRUE);

    /* Init timers of the call */
    pj_timer_entry_init(call->ringing_timer, 1, call, on_ringing_timer_callback);
    pj_timer_entry_init(call->media_session_timer, 1, call, on_media_state_timer_callback);

    pjsip_endpt_schedule_timer(machine->g_endpt, call->ringing_timer, &call->ringing_time);

    return PJ_TRUE;
}
