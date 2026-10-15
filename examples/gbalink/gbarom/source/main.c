#include <gba.h>
#include <stdio.h>

#define JOY_WRITE 2
#define JOY_READ 4
#define JOY_RW 6

int main(void) {
	irqInit();
	irqEnable(IRQ_VBLANK);

	consoleDemoInit();
	iprintf("\x1b[9;2HGBA Link Cable Controller\n");
	iprintf("\x1b[12;2HWaiting for N64\n");

	// Enable JOY BUS mode
	REG_JOYTR = 0;
	REG_HS_CTRL |= JOY_RW;
	REG_RCNT = R_JOYBUS;

	// Wait for first read command from N64
	while (true) {
		if(REG_HS_CTRL & JOY_READ) {
			REG_HS_CTRL |= JOY_RW;
			REG_JOYTR = 0x99bb4400;
			break;
		}
	}
	
	// Now just stream controller status
	while (true) {
		// Read held keys
		u16 keys = (REG_KEYINPUT & 0x03ff) ^ 0x03ff;
		iprintf("\x1b[12;2H 0x%04x / 0x%08lx\n", keys, (u32) keys);

		if(REG_HS_CTRL & JOY_READ) {
			REG_HS_CTRL |= JOY_RW;
			REG_JOYTR = (u32) keys;
		}
	}
}
