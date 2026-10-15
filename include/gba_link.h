/**
 * @file gba_link.h
 * @author TODO
 * @brief GBA Link Cable subsystem
 * @ingroup gba_link
 */

#ifndef LIBDRAGON_GBA_LINK_H
#define LIBDRAGON_GBA_LINK_H

#include <stdbool.h>

#include "preview.h"
#include "joypad.h"

/**
 * @defgroup gba_link GBA Link Cable Subsystem
 * @ingroup peripherals
 * @brief GBA Link Cable interface.
 * @author TODO
 *
 * The Nintendo GameCube Game Boy Advance Cable (DOL-011) is an accessory to
 * connect a GBA to a controller port on the GameCube. It allows in-game
 * communication between a GBA game and a GameCube game. The GameCube can
 * also leverage the Multi Boot feature to upload a rom to the GBA.
 *
 * Using a passive adapter, it is possible to connect the GBA Link Cable to
 * a N64.
 *
 * The GBA Link Cable subsystem provides an interface for communicating with
 * a GBA game. It also provides an implementation of Multi Boot to upload a
 * GBA rom.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

// TODO Detect GBA --> Active detection ? Specify port(s) allowed ?
// TODO Initialize / Upload with MultiBoot
// TODO Send / Receive words
// TODO Reset ?
// TODO Force reset for detection on a given port? --> async !


// TODO Documentation
/**
 * @brief GBA detect callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param gba_status        The status of the GBA as returned by it.
 */
typedef struct {
    /** @brief The Joybus port */
    int port;
    /** @brief Whether a GBA Link Cable was detected */
    bool is_gba_link;
    /** @brief Status byte returned by the GBA */
    uint8_t gba_status;
} gba_link_detect_t;


/**
 * @brief GBA detect callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param gba_status        The status of the GBA as returned by it.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_detect_callback_t)(gba_link_detect_t detected[4], void *ctx);


/**
 * @brief GBA read callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param data              The data sent by the GBA.
 * @param gba_status        The status of the GBA as returned by it.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_read_callback_t)(int port, const uint8_t data[4], uint8_t gba_status, void *ctx);

/**
 * @brief GBA write callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param gba_status        The status of the GBA as returned by it.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_write_callback_t)(int port, uint8_t gba_status, void *ctx);


// TODO Documentation
bool gba_link_is_connected(int port);

// TODO Documentation
bool gba_link_is_ready(int port);

/**
 * @brief Asynchronously detects any GBA Link.
 * 
 * @param callback A function pointer to call when the operation completes.
 * @param timeout Time (in ms) after which detection is aborted if no GBA Link Cable was identified.
 * @param ctx A user data pointer to pass into the callback function.
 */
void gba_link_detect_async(gba_link_detect_callback_t callback, int timeout, void *ctx);

/**
 * @brief Asynchronously perform a Joybus GBA Link read command.
 * 
 * @param port The controller port of the GBA Link to read from.
 * @param callback A function pointer to call when the operation completes.
 * @param ctx A user data pointer to pass into the callback function.
 */
void gba_link_read_async(int port, gba_link_read_callback_t callback, void *ctx);

/**
 * @brief Asynchronously perform a Joybus GBA Link write command.
 * 
 * @param port The controller port of the GBA Link to write to.
 * @param[in] data The data to write to the GBA Link.
 * @param callback A function pointer to call when the operation completes.
 * @param ctx A user data pointer to pass into the callback function.
 */
void gba_link_write_async(int port, const uint8_t *data, gba_link_write_callback_t callback, void *ctx);

/**
 * @brief Synchronously perform a Joybus GBA Link read command.
 * 
 * @param port The controller port of the GBA Link to read from.
 * @param[out] data The 4 bytes of data that was read from the GBA Link.
 *
 * @retval #GBA_LINK_IO_STATUS_OK The data was read successfully.
 * @retval #GBA_LINK_IO_STATUS_KO The data was not read successfully.
 */
int gba_link_read(int port, uint8_t *data);

/**
 * @brief Synchronously perform a Joybus GBA Link write command.
 * 
 * @param port The controller port of the GBA Link to write to.
 * @param[in] data The 4 bytes of data to write to the accessory.
 *
 * @retval #GBA_LINK_IO_STATUS_OK The data was written successfully.
 * @retval #GBA_LINK_IO_STATUS_KO The data was not written successfully.
 */
int gba_link_write(int port, const uint8_t *data);


#ifdef __cplusplus
}
#endif

/** @} */ /* gba_link */

#endif /* LIBDRAGON_GBA_LINK_H */
