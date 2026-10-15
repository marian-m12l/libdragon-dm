#include <stdbool.h>
#include <string.h>
#include <libdragon.h>


#define GBA_ROM_PATH "rom:/gbarom_mb.gba"

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

static volatile bool idenfify_pending = false;
static volatile int gba_port = -1;
static volatile bool multiboot_done = false;
static volatile uint32_t gba_controller_held;
static int x, y;

typedef enum KEYPAD_BITS {
	GBA_KEY_A		=	(1<<0),	/*!< keypad A button */
	GBA_KEY_B		=	(1<<1),	/*!< keypad B button */
	GBA_KEY_SELECT	=	(1<<2),	/*!< keypad SELECT button */
	GBA_KEY_START	=	(1<<3),	/*!< keypad START button */
	GBA_KEY_RIGHT	=	(1<<4),	/*!< dpad RIGHT */
	GBA_KEY_LEFT	=	(1<<5),	/*!< dpad LEFT */
	GBA_KEY_UP		=	(1<<6),	/*!< dpad UP */
	GBA_KEY_DOWN	=	(1<<7),	/*!< dpad DOWN */
	GBA_KEY_R		=	(1<<8),	/*!< Right shoulder button */
	GBA_KEY_L		=	(1<<9),	/*!< Left shoulder button */
} KEYPAD_BITS;

static void gba_link_multiboot_callback(int port, void *ctx) {
    // TODO Check status ok/ko
    multiboot_done = true;
}

static void gba_link_read_callback(int port, const uint8_t data[4], uint8_t gba_status, void *ctx) {
    // Received controller status
    gba_controller_held = __builtin_bswap32(*(uint32_t*)data);
    //printf("GBA controller: 0x%08lx\n", gba_controller_held);
}

static void gba_link_detect_callback(gba_link_detect_t detect[4], void *ctx) {
    printf("\n");
    for (int port = 0; port < 4; port++) {
        printf("PORT #%d: IS_GBA: %d Status: 0x%02x\n", detect[port].port, detect[port].is_gba_link, detect[port].gba_status);
        if (detect[port].is_gba_link) {
            gba_port = port;
            // Send ROM via multiboot, GBA logo blinking green to yellow
            gba_link_multiboot_start(port, GBA_ROM_PATH, GBA_LINK_LOGO_COLOR_3_GREEN, GBA_LINK_LOGO_SPEED_PLUS_1, gba_link_multiboot_callback, NULL);
            printf("GBA IN PORT: %d.\n", port);
            return;
        }
    }
    idenfify_pending = false;
}

int main(void)
{
    debug_init_emulog();
    debug_init_usblog();
    
    dfs_init(DFS_DEFAULT_LOCATION);

    asset_init_compression(2);

    console_init();
    console_set_render_mode(RENDER_MANUAL);
    console_set_debug(false);

    rdpq_init();
    rdpq_text_register_font(1, rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_VAR));

    joypad_init();

    console_clear();
    printf("\n");
    printf("LibDragon GBA Link Cable Subsystem Test\n");
    printf("\n");
    printf("Plug the GBA via its Link Cable to controller port 2-4\n");
    printf("Power on the GBA with no game\n");
    printf("(or hold SELECT+START to prevent booting the cartridge game)\n");
    printf("Press A to detect GBA Link Cable and upload a Multi Boot rom\n");

    while (!multiboot_done) {
        joypad_poll();
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        if (pressed.a && !idenfify_pending) {
            gba_port = -1;
            idenfify_pending = true;
            gba_link_detect_async(gba_link_detect_callback, 3000, NULL);
        }

        console_render();
    }

    // Wait for uploaded ROM to boot
    uint32_t data;
    do {
        gba_link_read(gba_port, (uint8_t*)&data);
    } while (data != 0x0044bb99);

    // Once GBA rom is sent via Multi Boot, read GBA controller status over JoyBus
    // GBA controls the text position
    display_close();
    display_init( RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_DISABLED );
    x = 160;
    y = 120;
    while (1) {
        // Acquire a free framebuffer for rendering
        surface_t *disp = display_get();
        // Attach the framebuffer for use by rdpq
        rdpq_attach(disp, NULL);

        // Clear the framebuffer with black
        rdpq_clear((color_t){0, 0, 0, 0});

        // Read from GBA
        gba_link_read_async(gba_port, gba_link_read_callback, NULL);

        if (gba_controller_held & GBA_KEY_UP)
            y = CLAMP(y - 2, 40, 200);
        if (gba_controller_held & GBA_KEY_DOWN)
            y = CLAMP(y + 2, 40, 200);
        if (gba_controller_held & GBA_KEY_LEFT)
            x = CLAMP(x - 2, 20, 220);
        if (gba_controller_held & GBA_KEY_RIGHT)
            x = CLAMP(x + 2, 20, 220);

        rdpq_text_print(NULL, 1, x, y, "Move me from GBA");

        rdpq_detach_show();
    }
}
