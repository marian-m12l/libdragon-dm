/**
 * @file gba_link.c
 * @author TODO
 * @brief GBA Link Cable Subsystem
 * @ingroup gba_link
 */

#include <stdlib.h>
#include <string.h>

#include "gba_link.h"
#include "joybus_commands.h"
#include "n64sys.h"
#include "timer.h"

// TODO One struct per port (support multiple GBAs)
// TODO Detect GBA --> Active detection ? Specify port(s) allowed ?
// TODO Force reset for detection on a given port? --> async !

#define IDENTIFY_RETRY_DELAY_US TIMER_TICKS(5000)

#define GBA_LINK_CONNECTED(port) \
    (joybus_get_identifier(port, NULL) == JOYBUS_IDENTIFIER_GBA_LINK_CABLE)

#define GBA_LINK_CONNECTED_STATUS(port, status) \
    (joybus_get_identifier(port, status) == JOYBUS_IDENTIFIER_GBA_LINK_CABLE)


// FIXME Copied from joybus.c --> joybus_internal.h/.c ??
static void joybus_input_identify( uint8_t input[JOYBUS_BLOCK_SIZE], bool reset )
{
    const joybus_cmd_identify_port_t cmd = { .send = {
        .command = reset ? JOYBUS_COMMAND_ID_RESET : JOYBUS_COMMAND_ID_IDENTIFY,
    } };
    const size_t recv_offset = offsetof(typeof(cmd), recv);
    size_t i = 0;

    // Populate the Joybus commands on each port
    memset(input, 0x00, JOYBUS_BLOCK_SIZE);
    for (int port = 0; port < JOYBUS_PORT_COUNT-1; port++)
    {
        // TODO Skip ports where a controller is already identified ?

        // iQue PIF requires a NOP (0xFF) before each command
        if (sys_bbplayer()) input[i++] = 0xFF;
        // Set the command metadata
        input[i++] = sizeof(cmd.send);
        input[i++] = sizeof(cmd.recv);
        // Micro-optimization: Minimize copy length
        memcpy(&input[i], &cmd, recv_offset);
        i += sizeof(cmd);
        // iQue PIF requires commands to be 8-byte aligned
        if (sys_bbplayer()) while (i & 7) input[i++] = 0xFF;
    }

    // Close out the Joybus operation block
    input[i] = 0xFE;
    input[JOYBUS_BLOCK_SIZE - 1] = 0x01;
}

typedef struct
{
    gba_link_detect_callback_t callback;
    bool done;
    timer_link_t *retry_timer;
    timer_link_t *abort_timer;
    void *ctx;
} gba_link_cable_detect_context_t;

static void retry_identify(int ovfl, void *ctx);

static void gba_link_identify_callback(uint64_t *out_dwords, void *ctx) {
    const uint8_t *out_bytes = (void *)out_dwords;
    gba_link_cable_detect_context_t* context = (void*)ctx;
    gba_link_detect_t detect[4];
    memset(detect, 0, sizeof(detect));
    size_t i = 0;
    for (int port = 0; port < 4; port++) {
        if (sys_bbplayer())
            i = (port * 8) + 1;
        const joybus_cmd_identify_port_t *cmd = (void *)&out_bytes[i + JOYBUS_COMMAND_METADATA_SIZE];

        detect[port].port = port;
        if ((out_bytes[i+1] & 0x80) == 0 && cmd->recv.identifier == JOYBUS_IDENTIFIER_GBA_LINK_CABLE && cmd->recv.status & JOYBUS_IDENTIFY_STATUS_GBA_GPIO0) {
            detect[port].is_gba_link = true;
            detect[port].gba_status = cmd->recv.status;
            context->done = true;
        }

        i += JOYBUS_COMMAND_METADATA_SIZE + sizeof(*cmd);
    }

    if (context->done) {
        if (context->abort_timer != NULL) {
            delete_timer(context->abort_timer);
            timer_close();
        }
        gba_link_detect_callback_t callback = context->callback;
        if (callback != NULL)
            callback(detect, (void*)context->ctx);
        free(context);
    } else {
        timer_init();
        context->retry_timer = new_timer_context(IDENTIFY_RETRY_DELAY_US, TF_ONE_SHOT, retry_identify, context);
    }
}

static void retry_identify(int ovfl, void *ctx) {
    gba_link_cable_detect_context_t* context = (void*)ctx;
    delete_timer(context->retry_timer);
    timer_close();
    context->retry_timer = NULL;
    uint8_t input[JOYBUS_BLOCK_SIZE] = {0};
    // First send a reset command
    joybus_input_identify(input, true);
    joybus_exec_async(input, NULL, NULL);
    // Then send an identify command
    joybus_input_identify(input, false);
    joybus_exec_async(input, gba_link_identify_callback, ctx);
}

static void abort_identify(int ovfl, void *ctx) {
    gba_link_cable_detect_context_t* context = (void*)ctx;
    delete_timer(context->abort_timer);
    timer_close();
    context->abort_timer = NULL;
    context->done = true;
}

void gba_link_detect_async(gba_link_detect_callback_t callback, int timeout, void* ctx)
{
    // TODO Async operations are disabled during reset ?
    // TODO Bail if this operation is already in-progress
    uint8_t input[JOYBUS_BLOCK_SIZE] = {0};
    // First send a reset command
    joybus_input_identify(input, true);
    joybus_exec_async(input, NULL, NULL);
    // Then send an identify command
    joybus_input_identify(input, false);
    gba_link_cable_detect_context_t* gba_link_detect_context = calloc(1, sizeof(gba_link_cable_detect_context_t));
    gba_link_detect_context->callback = callback;
    gba_link_detect_context->done = false;
    gba_link_detect_context->retry_timer = NULL;
    gba_link_detect_context->ctx = ctx;
    // Set abort timer
    timer_init();
    gba_link_detect_context->abort_timer = new_timer_context(TIMER_TICKS(timeout * 1000), TF_ONE_SHOT, abort_identify, (void*)gba_link_detect_context);
    joybus_exec_async(input, gba_link_identify_callback, (void*)gba_link_detect_context);
}


bool gba_link_is_connected(int port)
{
    return GBA_LINK_CONNECTED(port);
}

bool gba_link_is_ready(int port)
{
    uint8_t status;
    if (!GBA_LINK_CONNECTED_STATUS(port, &status)) { return false; }
    return status & JOYBUS_IDENTIFY_STATUS_GBA_GPIO0;
}

typedef struct
{
    int port;
    gba_link_read_callback_t callback;
    void *ctx;
} gba_link_cable_read_context_t;

static void gba_link_read_callback(uint64_t *out_dwords, void *ctx)
{
    const uint8_t *out_bytes = (void *)out_dwords;
    gba_link_cable_read_context_t* context = (void*)ctx;
    joypad_port_t port = context->port;

    const joybus_cmd_gba_link_cable_read_port_t *cmdr =
        (void *)&out_bytes[port + JOYBUS_COMMAND_METADATA_SIZE];

    // TODO Check GBA status (what is expected ??? NOT 0x02 ??)

    gba_link_read_callback_t callback = context->callback;
    if (callback != NULL)
        callback(port, cmdr->recv.data, cmdr->recv.status, (void*)context->ctx);
    free(context);
}

void gba_link_read_async(
    int port,
    gba_link_read_callback_t callback,
    void *ctx
)
{
    uint8_t input[JOYBUS_BLOCK_SIZE] = {0};
    joybus_cmd_gba_link_cable_read_port_t cmd = { .send = {
        .command = JOYBUS_COMMAND_ID_GBA_LINK_CABLE_READ,
    } };
    size_t i = port;
    // Set the command metadata
    input[i++] = sizeof(cmd.send);
    input[i++] = sizeof(cmd.recv);
    // Micro-optimization: Minimize copy length
    const size_t recv_offset = offsetof(typeof(cmd), recv);
    memcpy(&input[i], &cmd.send, recv_offset);
    i += sizeof(cmd);
    // Close out the Joybus operation block
    input[i] = 0xFE;
    input[sizeof(input) - 1] = 0x01;
    // Execute the Joybus operation asynchronously
    gba_link_cable_read_context_t* gba_link_read_context = calloc(1, sizeof(gba_link_cable_read_context_t));
    gba_link_read_context->port = port;
    gba_link_read_context->callback = callback;
    gba_link_read_context->ctx = ctx;
    joybus_exec_async(input, gba_link_read_callback, (void*)gba_link_read_context);
}

typedef struct
{
    int port;
    gba_link_write_callback_t callback;
    void *ctx;
} gba_link_cable_write_context_t;

static void gba_link_write_callback(uint64_t *out_dwords, void *ctx)
{
    const uint8_t *out_bytes = (void *)out_dwords;
    gba_link_cable_write_context_t* context = (void*)ctx;
    joypad_port_t port = context->port;

    const joybus_cmd_gba_link_cable_write_port_t *cmdw =
        (void *)&out_bytes[port + JOYBUS_COMMAND_METADATA_SIZE];

    // TODO Check GBA status (what is expected ??? NOT 0x02 ??)

    gba_link_write_callback_t callback = context->callback;
    if (callback != NULL)
        callback(port, cmdw->recv.status, (void*)context->ctx);
    free(context);
}

void gba_link_write_async(
    int port,
    const uint8_t *data,
    gba_link_write_callback_t callback,
    void *ctx
)
{
    uint8_t input[JOYBUS_BLOCK_SIZE] = {0};
    joybus_cmd_gba_link_cable_write_port_t cmd = { .send = {
        .command = JOYBUS_COMMAND_ID_GBA_LINK_CABLE_WRITE,
    } };
    size_t i = port;
    // Set the command metadata
    input[i++] = sizeof(cmd.send);
    input[i++] = sizeof(cmd.recv);
    // Micro-optimization: Minimize copy length
    const size_t data_offset = offsetof(typeof(cmd.send), data);
    memcpy(&input[i], &cmd.send, data_offset);
    memcpy(&input[i + data_offset], data, sizeof(cmd.send.data));
    i += sizeof(cmd);
    // Close out the Joybus operation block
    input[i] = 0xFE;
    input[sizeof(input) - 1] = 0x01;
    // Execute the Joybus operation asynchronously
    gba_link_cable_write_context_t* gba_link_write_context = calloc(1, sizeof(gba_link_cable_write_context_t));
    gba_link_write_context->port = port;
    gba_link_write_context->callback = callback;
    gba_link_write_context->ctx = ctx;
    joybus_exec_async(input, gba_link_write_callback, (void*)gba_link_write_context);
}

int gba_link_read(int port, uint8_t *data)
{
    joybus_cmd_gba_link_cable_read_port_t cmd = { .send = {
        .command = JOYBUS_COMMAND_ID_GBA_LINK_CABLE_READ,
    } };
    // Execute the command
    joybus_exec_cmd_struct(port, cmd);
    // Copy the data from the command receive buffer
    memcpy(data, cmd.recv.data, sizeof(cmd.recv.data));
    // TODO Check GBA status (what is expected ??? NOT 0x02 ??)
    return 0;
}

int gba_link_write(int port, const uint8_t *data)
{
    joybus_cmd_gba_link_cable_write_port_t cmd = { .send = {
        .command = JOYBUS_COMMAND_ID_GBA_LINK_CABLE_WRITE,
    } };
    // Copy the data to the command send buffer
    memcpy(cmd.send.data, data, sizeof(cmd.send.data));
    // Execute the command
    joybus_exec_cmd_struct(port, cmd);
    // TODO Check GBA status (what is expected ??? NOT 0x02 ??)
    return 0;
}

/** @} */ /* gba_link */
