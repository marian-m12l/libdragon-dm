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

typedef enum {
	GBA_LINK_LOGO_COLOR_0_PINK = 0,
	GBA_LINK_LOGO_COLOR_1_BLUE,
	GBA_LINK_LOGO_COLOR_2_CYAN,
	GBA_LINK_LOGO_COLOR_3_GREEN,
	GBA_LINK_LOGO_COLOR_4_YELLOW,
	GBA_LINK_LOGO_COLOR_5_RED,
	GBA_LINK_LOGO_COLOR_6_PINK,
	// WHITE
} gba_link_logo_color_t;

typedef enum {
	GBA_LINK_LOGO_SPEED_MINUS_4 = -4,
	GBA_LINK_LOGO_SPEED_MINUS_3,
	GBA_LINK_LOGO_SPEED_MINUS_2,
	GBA_LINK_LOGO_SPEED_MINUS_1,
	GBA_LINK_LOGO_SPEED_0,
	GBA_LINK_LOGO_SPEED_PLUS_1,
	GBA_LINK_LOGO_SPEED_PLUS_2,
	GBA_LINK_LOGO_SPEED_PLUS_3,
	GBA_LINK_LOGO_SPEED_PLUS_4,
} gba_link_logo_speed_t;

/**
 * @brief GBA Multi Boot callback function type.
 * 
 * TODO Documentation
 * TODO Add fields: status ok/ko ??
 * 
 * @param port              The Joybus port the GBA is connected to.
 * @param ctx               User-defined context pointer.
 */
typedef void (*gba_link_multiboot_callback_t)(int port, void *ctx);

// TODO Documentation
void gba_link_multiboot_start(int port, const char* file_name, gba_link_logo_color_t logo_color, gba_link_logo_speed_t logo_speed, gba_link_multiboot_callback_t callback, void *ctx);


#ifdef __cplusplus
}
#endif

/** @} */ /* gba_link */

#endif /* LIBDRAGON_GBA_LINK_MULTIBOOT_H */
