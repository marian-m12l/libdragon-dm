/**
 * @file gba_link.h
 * @author TODO
 * @brief GBA Link Cable subsystem
 * @ingroup gba_link
 */

#ifndef LIBDRAGON_GBA_LINK_H
#define LIBDRAGON_GBA_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "joybus.h"

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


/** @brief GBA Link Cable read/write status values */
typedef enum
{
    /** @brief GBA Link Cable data communcation was successful. */
    GBA_LINK_IO_STATUS_OK        = 0,
    /** @brief GBA Link Cable device is absent or did not respond. */
    GBA_LINK_IO_STATUS_ABSENT,
    /** @brief GBA Link Cable's reported status indicates that GBA was not ready to send data. */
    GBA_LINK_IO_STATUS_DATA_NOT_READY,
    /** @brief GBA Link Cable's reported status is unexpected for the current I/O operation. */
    GBA_LINK_IO_STATUS_UNEXPECTED_STATUS
} gba_link_io_status_t;

/**
 * @brief GBA read callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param data              The data sent by the GBA.
 * @param gba_status        The status of the GBA as returned by it.
 * @param io_status         Indicates whether the operation was successful or not.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_read_callback_t)(int port, const uint8_t data[JOYBUS_GBA_LINK_CABLE_DATA_SIZE], uint8_t gba_status, gba_link_io_status_t io_status, void *ctx);

/**
 * @brief GBA write callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param gba_status        The status of the GBA as returned by it.
 * @param io_status         Indicates whether the operation was successful or not.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_write_callback_t)(int port, uint8_t gba_status, gba_link_io_status_t io_status, void *ctx);


// TODO Documentation
bool gba_link_is_connected(int port);

// TODO Documentation
uint8_t gba_link_get_status(int port);

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
 * @param[out] gba_status The status of the GBA as returned by it.
 *
 * @retval #GBA_LINK_IO_STATUS_OK The data was read successfully.
 * @retval #GBA_LINK_IO_STATUS_ABSENT The data was not read successfully because the device is absent.
 * @retval #GBA_LINK_IO_STATUS_DATA_NOT_READY The data was not read successfully because the device is not ready.
 * @retval #GBA_LINK_IO_STATUS_UNEXPECTED_STATUS The data was not read successfully because the device is in an invalid state.
 */
gba_link_io_status_t gba_link_read(int port, uint8_t *data, uint8_t* gba_status);

/**
 * @brief Synchronously perform a Joybus GBA Link write command.
 * 
 * @param port The controller port of the GBA Link to write to.
 * @param[in] data The 4 bytes of data to write to the accessory.
 * @param[out] gba_status The status of the GBA as returned by it.
 *
 * @retval #GBA_LINK_IO_STATUS_OK The data was written successfully.
 * @retval #GBA_LINK_IO_STATUS_ABSENT The data was not written successfully because the device is absent.
 * @retval #GBA_LINK_IO_STATUS_UNEXPECTED_STATUS The data was not written successfully because the device is in an invalid state.
 */
gba_link_io_status_t gba_link_write(int port, const uint8_t *data, uint8_t* gba_status);


#ifdef __cplusplus
}
#endif

/** @} */ /* gba_link */

#endif /* LIBDRAGON_GBA_LINK_H */
