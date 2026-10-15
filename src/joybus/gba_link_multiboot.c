/**
 * @file gba_link_multiboot.c
 * @author TODO
 * @brief GBA Link Cable support for Multi Boot
 * @ingroup gba_link
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asset.h"
#include "gba_link_multiboot.h"


// TODO Simplify functions calc_key, update_crc, and cipher_data

#define ROUND_UP_8(val) (((val) + 7) & ~7)

// TODO char* keys = " by Kawasedo";
#define MAGIC_SEDO 0x7365646f  // "odes" or "sedo" ?
#define MAGIC_KAWA 0x6177614b  // "awaK" or "Kawa" ?
#define MAGIC_BY 0x20796220  // " yb " or " by " ?

uint32_t calc_key(uint32_t size, gba_link_logo_speed_t speed, gba_link_logo_color_t color) {
	uint32_t ret = 0;
	size = (size-0x200) >> 3;
	//uint32_t pal = 0x2e << 16;	// Color 2 (0..6) Speed -4 (-4..4)
	uint32_t pal = speed < 0
		? ((color << 4 | (3 - speed)*2))
		: (speed > 0
			? (color << 4 | (speed - 1)*2)
			: (color << 1 | 0x70)
		);
	pal <<= 16;
	int res1 = pal;
	res1 |= (size&0x3F80) << 1;
	res1 |= (size&0x4000) << 2;
	res1 |= (size&0x7F);
	int res2 = res1;
	res1 = res2 >> 0x10;
	int res3 = res2 >> 8;
	res3 += res1;
	res3 += res2;
	res3 <<= 24;
	res3 |= res2;
	res3 |= 0x80808080;

    // FIXME Compare with __builtin_bswap32(res3 ^ 0x6177614B); // "kawa" --> LE & BE !!
    // FIXME Compare with __builtin_bswap32(res3 ^ 0x6F646573); // "sedo" --> LE & BE !!
    // TODO swap ??
    //return __builtin_bswap32(res3 ^ ((res3 & 0x200) != 0 ? MAGIC_SEDO : __builtin_bswap32(MAGIC_KAWA)));

	if((res3&0x200) == 0) {
		ret |= (((res3)&0xFF)^0x4B)<<24;
		ret |= (((res3>>8)&0xFF)^0x61)<<16;
		ret |= (((res3>>16)&0xFF)^0x77)<<8;
		ret |= (((res3>>24)&0xFF)^0x61);
	} else {
		ret |= (((res3)&0xFF)^0x73)<<24;
		ret |= (((res3>>8)&0xFF)^0x65)<<16;
		ret |= (((res3>>16)&0xFF)^0x64)<<8;
		ret |= (((res3>>24)&0xFF)^0x6F);
	}
	return ret;
}

uint32_t update_crc(uint32_t crc, uint32_t val) {
    for (int i = 0; i < 0x20; i++) {
        if ((crc ^ val) & 1) {
			crc >>= 1;
			crc ^= 0xa1c1;
        } else {
			crc >>= 1;
        }
		val >>= 1;
	}
	return crc;
}

uint32_t cipher_data(uint32_t data, int i, uint32_t* sessionkey) {
    *sessionkey = (*sessionkey * 0x6177614B) + 1;   // "Kawa"
    // *sessionkey = (*sessionkey * 0x4b617761) + 1;   // "Kawa"
    data ^= *sessionkey;
    data ^= ((~(i + (0x20<<20))) + 1);
    data ^= 0x20796220; // " by "
    //data ^= 0x20627920; // " by "
    return data;
}


typedef enum {
    GBA_MULTIBOOT_IDLE = 0,
    GBA_MULTIBOOT_CLEARTEXT,
    GBA_MULTIBOOT_CIPHERTEXT,
    GBA_MULTIBOOT_CRC,
    GBA_MULTIBOOT_CHECK,
    GBA_MULTIBOOT_DONE,
} multiboot_state_t;

typedef struct
{
    FILE* file;
    int length;
    gba_link_logo_color_t logo_color;
    gba_link_logo_speed_t logo_speed;
    multiboot_state_t state;
    uint32_t crc;
    uint32_t sessionkey;
    int sent;
    gba_link_multiboot_callback_t callback;
    void *ctx;
} multiboot_context_t;


// TODO Check GBA status byte and handle errors (invalid GBA status, file read failure, ...) --> ferror(context->file) + fclose(context->file) + callback with failure

static void gba_link_write_callback(int port, uint8_t gba_status, void *ctx);

static void gba_link_read_callback(int port, const uint8_t data[4], uint8_t gba_status, void *ctx) {
    multiboot_context_t* context = (void*) ctx;
    switch (context->state) {
        case GBA_MULTIBOOT_IDLE: {
            // Compute session key from received data
            context->sessionkey = *(uint32_t*)data;
            context->sessionkey = context->sessionkey ^ MAGIC_SEDO;
            context->sessionkey = __builtin_bswap32(context->sessionkey);
            // Compute host key from payload length and palette configuration
            uint32_t calc = calc_key(ROUND_UP_8(context->length), context->logo_speed, context->logo_color);
            gba_link_write_async(port, (uint8_t*) &calc, gba_link_write_callback, ctx);
            context->crc = 0x15a0;
            context->sent = 0;
            context->state = GBA_MULTIBOOT_CLEARTEXT;
            break;
        }
        case GBA_MULTIBOOT_CLEARTEXT:
        case GBA_MULTIBOOT_CIPHERTEXT:
        case GBA_MULTIBOOT_CRC:
            break;
        case GBA_MULTIBOOT_CHECK: {
            // TODO Check GBA response ? Read twice (dummy bytes, initial code) ??
            context->state = GBA_MULTIBOOT_DONE;
            gba_link_multiboot_callback_t callback = context->callback;
            if (callback != NULL)
                callback(port, (void*)context->ctx);
            fclose(context->file);
            free(context);
            break;
        }
        case GBA_MULTIBOOT_DONE: {
            break;
        }
    }
}

static void gba_link_write_callback(int port, uint8_t gba_status, void *ctx) {
    multiboot_context_t* context = (void*) ctx;
    switch (context->state) {
        case GBA_MULTIBOOT_IDLE:
            break;
        case GBA_MULTIBOOT_CLEARTEXT: {
            uint8_t buf[4];
            size_t nread;
            if ((nread = fread(buf, sizeof(buf[0]), sizeof(buf), context->file)) != 0) {
                gba_link_write_async(port, buf, gba_link_write_callback, ctx);
                context->sent += 4;
                if (context->sent == 0xc0) {
                    context->state = GBA_MULTIBOOT_CIPHERTEXT;
                }
            }
            // TODO Handle errors: if (ferror(context->file)) + fclose(context->file) + callback with failure
            // TODO Status: GPIO0 should be toggled after each write command ??
            break;
        }
        case GBA_MULTIBOOT_CIPHERTEXT: {
            // ROM data must be ciphered for the remaining of the payload
            // CRC is updated for each packet sent
            // FIXME avoid swapping twice
            uint8_t buf[4];
            size_t nread;
            if ((nread = fread(buf, sizeof(buf[0]), sizeof(buf), context->file)) != 0) {
                uint32_t d = __builtin_bswap32(*(uint32_t*)buf);
                uint32_t cipherText = __builtin_bswap32(cipher_data(d, context->sent, &context->sessionkey));
                context->crc = update_crc(context->crc, d);
                gba_link_write_async(port, (uint8_t*) &cipherText, gba_link_write_callback, ctx);
                context->sent += 4;
                if (context->sent == ROUND_UP_8(context->length)) {
                    context->state = GBA_MULTIBOOT_CRC;
                }
            }
            // TODO Handle errors: if (ferror(context->file)) + fclose(context->file) + callback with failure
            break;
        }
        case GBA_MULTIBOOT_CRC: {
            // Finalize and send CRC
            context->crc |= (ROUND_UP_8(context->length) << 16);
            context->crc = __builtin_bswap32(cipher_data(context->crc, context->sent, &context->sessionkey));
            gba_link_write_async(port, (uint8_t*) &context->crc, gba_link_write_callback, ctx);
            context->state = GBA_MULTIBOOT_CHECK;
            break;
        }
        case GBA_MULTIBOOT_CHECK: {
            gba_link_read_async(port, gba_link_read_callback, ctx);
            break;
        }
        case GBA_MULTIBOOT_DONE:
            break;
    }
}

void gba_link_multiboot_start(
    int port,
    const char *file_name,
    gba_link_logo_color_t logo_color,
    gba_link_logo_speed_t logo_speed,
    gba_link_multiboot_callback_t callback,
    void *ctx
)
{
    multiboot_context_t* multiboot_context = calloc(1, sizeof(multiboot_context_t));
    multiboot_context->callback = callback;
    multiboot_context->ctx = ctx;
    multiboot_context->file = asset_fopen(file_name, &multiboot_context->length);
    multiboot_context->logo_color = logo_color;
    multiboot_context->logo_speed = logo_speed;
    gba_link_read_async(port, gba_link_read_callback, (void*)multiboot_context);
}

/** @} */ /* gba_link */
