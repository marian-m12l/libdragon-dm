/**
 * @file gba_link_multiboot.h
 * @author TODO
 * @brief GBA Link Cable support for Multi Boot
 * @ingroup gba_link
 */

#ifndef LIBDRAGON_GBA_LINK_MULTIBOOT_H
#define LIBDRAGON_GBA_LINK_MULTIBOOT_H

#include <stdbool.h>

#include "preview.h"
#include "gba_link.h"

/**
 * @addtogroup gba_link
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif


/** @brief GBA Multi Boot status values */
typedef enum
{
    /** @brief GBA Multi Boot was successful. */
    GBA_LINK_MULTIBOOT_OK        = 0,
    /** @brief GBA Multi Boot failed because of an issue in I/O operations. */
    GBA_LINK_MULTIBOOT_BAD_IO,
    /** @brief GBA Multi Boot failed because the GBA reported a status inconsistent with I/O operations. */
    GBA_LINK_MULTIBOOT_BAD_STATUS,
    /** @brief GBA Multi Boot failed because of an issue while reading file. */
    GBA_LINK_MULTIBOOT_BAD_FILE
} gba_link_multiboot_status_t;


// TODO Documentation

// | color | speed | result |
// |-------|-------|--------|
// | 0     | +1    | pink to blue |
// | 0     | -1    | blue to pink |
// | 1     | +1    | blue to cyan |
// | 1     | -1    | cyan to blue |
// | 2     | +1    | cyan to green |
// | 2     | -1    | green to cyan |
// | 3     | +1    | green to yellow |
// | 3     | -1    | yellow to green |
// | 4     | +1    | yellow to red |
// | 4     | -1    | red to yellow |
// | 5     | +1    | red to pink |
// | 5     | -1    | pink to red |
// | 6     | +1    | pink to white |
// | 6     | -1    | white to pink |

typedef enum : uint8_t {
	GBA_LINK_MULTIBOOT_LOGO_COLOR_0_PINK = 0,
	GBA_LINK_MULTIBOOT_LOGO_COLOR_1_BLUE,
	GBA_LINK_MULTIBOOT_LOGO_COLOR_2_CYAN,
	GBA_LINK_MULTIBOOT_LOGO_COLOR_3_GREEN,
	GBA_LINK_MULTIBOOT_LOGO_COLOR_4_YELLOW,
	GBA_LINK_MULTIBOOT_LOGO_COLOR_5_RED,
	GBA_LINK_MULTIBOOT_LOGO_COLOR_6_PINK,
	// WHITE
} gba_link_multiboot_logo_color_t;

typedef enum : int8_t {
	GBA_LINK_MULTIBOOT_LOGO_SPEED_MINUS_4 = -4,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_MINUS_3,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_MINUS_2,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_MINUS_1,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_0,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_PLUS_1,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_PLUS_2,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_PLUS_3,
	GBA_LINK_MULTIBOOT_LOGO_SPEED_PLUS_4,
} gba_link_multiboot_logo_speed_t;

/**
 * @brief GBA Multi Boot callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param status			Multiboot execution status
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_multiboot_callback_t)(int port, gba_link_multiboot_status_t status, void *ctx);

// TODO Documentation
void gba_link_multiboot_start(int port, const char* file_name, gba_link_multiboot_logo_color_t logo_color, gba_link_multiboot_logo_speed_t logo_speed, gba_link_multiboot_callback_t callback, void *ctx);


/**
 * @brief GBA Multi Boot detection callback function type.
 * 
 * TODO Documentation
 * 
 * @param port              The Joybus port on which GBA detection happened.
 * @param multiboot_ready   Wheteher a GBA Link Cable is connected to the given port and ready for Multi Boot.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_multiboot_detect_callback_t)(int port, bool multiboot_ready, void *ctx);

/**
 * @brief Asynchronously detect a GBA Link Cable ready for Multi Boot on the given port.
 * 
 * Emits reset and identify commands for a given time, checking the GBA status.
 * 
 * TODO Documentation
 * 
 * @param port The Joybus port on which to detect GBA Link Cable.
 * @param timeout Time (in ms) after which detection is aborted if no GBA Link Cable was identified.
 * @param callback A function pointer to call when the operation completes.
 * @param ctx A user data pointer to pass into the callback function.
 */
void gba_link_multiboot_detect(int port, int timeout, gba_link_multiboot_detect_callback_t callback, void *ctx);

// TODO Documentation
uint8_t gba_link_multiboot_progress(int port);


#ifdef __cplusplus
}
#endif

/** @} */ /* gba_link */

#endif /* LIBDRAGON_GBA_LINK_MULTIBOOT_H */
