#pragma once

#define CHIPS_IMPL
#include "Z80.h"
#include "Rom.h"
#include "Clock.h"
#include "Memory.h"
#include "Keyboard.h"

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (256)
#define DISPLAY_PIXEL_BYTES sizeof(uint32_t)

#define DISPLAY_BYTES (DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_PIXEL_BYTES)

static uint32_t _zx_palette[8] =
{
    0xFF000000,     // black
    0xFFFF0000,     // blue
    0xFF0000FF,     // red
    0xFFFF00FF,     // magenta
    0xFF00FF00,     // green
    0xFFFFFF00,     // cyan
    0xFF00FFFF,     // yellow
    0xFFFFFFFF,     // white
};

typedef struct
{
    void* pixel_buffer;
    int pixel_buffer_size;
    void* user_data;
} zx_desc_t;

typedef struct
{
    z80_t cpu;
    bool valid;
    uint8_t kbd_joymask;
    uint8_t joy_joymask;
    uint8_t last_mem_config;
    uint8_t last_fe_out;
    uint8_t blink_counter;
    int frame_scan_lines;
    int top_border_scanlines;
    int scanline_period;
    int scanline_counter;
    int scanline_y;
    uint32_t display_ram_bank;
    uint32_t border_color;
    clk_t clk;
    kbd_t kbd;
    mem_t mem;
    uint32_t* pixel_buffer;
    void* user_data;
    uint8_t ram[8][0x4000];
} zx_t;

void zx_init(zx_t* sys, const zx_desc_t* desc);
void zx_exec(zx_t* sys, uint32_t micro_seconds);
static uint64_t _zx_tick(int num, uint64_t pins, void* user_data);
static bool _zx_decode_scanline(zx_t* sys);
static void _zx_init_memory_map(zx_t* sys);
static void _zx_init_keyboard_matrix(zx_t* sys);

#define _ZX_DEFAULT(val,def) (((val) != 0) ? (val) : (def));
#define _ZX_CLEAR(val) memset(&val, 0, sizeof(val))

void zx_init(zx_t* sys, const zx_desc_t* desc)
{
    CHIPS_ASSERT(sys && desc);
    CHIPS_ASSERT(desc->pixel_buffer && (desc->pixel_buffer_size >= DISPLAY_PIXEL_BYTES));

    memset(sys, 0, sizeof(zx_t));
    sys->valid = true;
    sys->pixel_buffer = (uint32_t*)desc->pixel_buffer;
    sys->user_data = desc->user_data;

    sys->display_ram_bank = 0;
    sys->frame_scan_lines = 312;
    sys->top_border_scanlines = 64;
    sys->scanline_period = 224;

    sys->scanline_counter = sys->scanline_period;

    const int cpu_freq = 3500000;
    clk_init(&sys->clk, cpu_freq);

    z80_desc_t cpu_desc;
    _ZX_CLEAR(cpu_desc);
    cpu_desc.tick_cb = _zx_tick;
    cpu_desc.user_data = sys;
    z80_init(&sys->cpu, &cpu_desc);

    _zx_init_memory_map(sys);
    _zx_init_keyboard_matrix(sys);

    z80_set_pc(&sys->cpu, 0x0000);
}

void zx_exec(zx_t* sys, uint32_t micro_seconds)
{
    CHIPS_ASSERT(sys && sys->valid);
    uint32_t ticks_to_run = clk_ticks_to_run(&sys->clk, micro_seconds);
    uint32_t ticks_executed = z80_exec(&sys->cpu, ticks_to_run);
    clk_ticks_executed(&sys->clk, ticks_executed);
    kbd_update(&sys->kbd, micro_seconds);
}

static void _zx_init_memory_map(zx_t* sys)
{
    mem_init(&sys->mem);
    mem_map_ram(&sys->mem, 0, 0x4000, 0x4000, sys->ram[0]);
    mem_map_ram(&sys->mem, 0, 0x8000, 0x4000, sys->ram[1]);
    mem_map_ram(&sys->mem, 0, 0xC000, 0x4000, sys->ram[2]);
    mem_map_rom(&sys->mem, 0, 0x0000, 0x4000, &rom48k[0]);
}

static void _zx_init_keyboard_matrix(zx_t* sys)
{
    kbd_init(&sys->kbd, 1);
    kbd_register_modifier(&sys->kbd, 0, 0, 0);
    kbd_register_modifier(&sys->kbd, 1, 7, 1);
    const char* keymap =
        // no shift
        " zxcv"         // A8       shift,z,x,c,v
        "asdfg"         // A9       a,s,d,f,g
        "qwert"         // A10      q,w,e,r,t
        "12345"         // A11      1,2,3,4,5
        "09876"         // A12      0,9,8,7,6
        "poiuy"         // A13      p,o,i,u,y
        " lkjh"         // A14      enter,l,k,j,h
        "  mnb"         // A15      space,symshift,m,n,b

        // shift
        " ZXCV"         // A8
        "ASDFG"         // A9
        "QWERT"         // A10
        "     "         // A11
        "     "         // A12
        "POIUY"         // A13
        " LKJH"         // A14
        "  MNB"         // A15

        // symshift
        " : ?/"         // A8
        "     "         // A9
        "   <>"         // A10
        "!@#$%"         // A11
        "_)('&"         // A12
        "\";   "        // A13
        " =+-^"         // A14
        "  .,*";        // A15

    for (int layer = 0; layer < 3; layer++)
    {
        for (int column = 0; column < 8; column++)
        {
            for (int line = 0; line < 5; line++)
            {
                const uint8_t c = keymap[layer * 40 + column * 5 + line];
                if (c != 0x20)
                {
                    kbd_register_key(&sys->kbd, c, column, line, (layer > 0) ? (1 << (layer - 1)) : 0);
                }
            }
        }
    }

    // special keys
    kbd_register_key(&sys->kbd, ' ', 7, 0, 0);  // Space
    kbd_register_key(&sys->kbd, 0x0F, 7, 1, 0); // SymShift
    kbd_register_key(&sys->kbd, 0x08, 3, 4, 1); // Cursor Left (Shift+5)
    kbd_register_key(&sys->kbd, 0x0A, 4, 4, 1); // Cursor Down (Shift+6)
    kbd_register_key(&sys->kbd, 0x0B, 4, 3, 1); // Cursor Up (Shift+7)
    kbd_register_key(&sys->kbd, 0x09, 4, 2, 1); // Cursor Right (Shift+8)
    kbd_register_key(&sys->kbd, 0x07, 3, 0, 1); // Edit (Shift+1)
    kbd_register_key(&sys->kbd, 0x0C, 4, 0, 1); // Delete (Shift+0)
    kbd_register_key(&sys->kbd, 0x0D, 6, 0, 0); // Enter
}

static uint64_t _zx_tick(int num_ticks, uint64_t pins, void* user_data)
{
    zx_t* sys = (zx_t*)user_data;
    // video decoding and vblank interrupt
    sys->scanline_counter -= num_ticks;
    if (sys->scanline_counter <= 0)
    {
        sys->scanline_counter += sys->scanline_period;
        // decode next video scanline
        if (_zx_decode_scanline(sys))
        {
            // request vblank interrupt
            pins |= Z80_INT;
        }
    }

    // memory and IO requests
    if (pins & Z80_MREQ)
    {
        const uint16_t addr = Z80_GET_ADDR(pins);
        if (pins & Z80_RD)
        {
            Z80_SET_DATA(pins, mem_rd(&sys->mem, addr));
        }
        else if (pins & Z80_WR)
        {
            mem_wr(&sys->mem, addr, Z80_GET_DATA(pins));
        }
    }
    else if (pins & Z80_IORQ)
    {
        if (pins & Z80_RD)
        {
            if ((pins & Z80_A0) == 0) {
                uint8_t data = (1 << 7) | (1 << 5);
                // MIC/EAR flags -> bit 6
                if (sys->last_fe_out & (1 << 3 | 1 << 4))
                {
                    data |= (1 << 6);
                }
                // keyboard matrix bits are encoded in the upper 8 bit of the port address
                uint16_t column_mask = (~(Z80_GET_ADDR(pins) >> 8)) & 0x00FF;
                const uint16_t kbd_lines = kbd_test_lines(&sys->kbd, column_mask);
                data |= (~kbd_lines) & 0x1F;
                Z80_SET_DATA(pins, data);
            }
            else if ((pins & (Z80_A7 | Z80_A6 | Z80_A5)) == 0)
            {
                // Kempston Joystick (........000.....)
                Z80_SET_DATA(pins, sys->kbd_joymask | sys->joy_joymask);
            }
        }
        else if (pins & Z80_WR)
        {
            const uint8_t data = Z80_GET_DATA(pins);
            if ((pins & Z80_A0) == 0)
            {
                sys->border_color = _zx_palette[data & 7] & 0xFFD7D7D7;
                sys->last_fe_out = data;
            }
        }
    }
    return pins;
}

static bool _zx_decode_scanline(zx_t* sys)
{
    const int top_decode_line = sys->top_border_scanlines - 32;
    const int btm_decode_line = sys->top_border_scanlines + 192 + 32;

    if ((sys->scanline_y >= top_decode_line) && (sys->scanline_y < btm_decode_line))
    {
        const uint16_t y = sys->scanline_y - top_decode_line;
        uint32_t* dst = &sys->pixel_buffer[y * DISPLAY_WIDTH];
        const uint8_t* vidmem_bank = sys->ram[sys->display_ram_bank];
        const bool blink = 0 != (sys->blink_counter & 0x10);
        uint32_t fg, bg;

        if ((y < 32) || (y >= 224))
        {
            // upper/lower border
            for (int x = 0; x < DISPLAY_WIDTH; x++)
            {
                *dst++ = sys->border_color;
            }
        }
        else
        {
            // compute video memory Y offset (inside 256x192 area)
            //  this is how the 16-bit video memory address is computed
            //  from X and Y coordinates:
            //  | 0| 1| 0|Y7|Y6|Y2|Y1|Y0|Y5|Y4|Y3|X4|X3|X2|X1|X0|
            //
            const uint16_t yy = y - 32;
            const uint16_t y_offset = ((yy & 0xC0) << 5) | ((yy & 0x07) << 8) | ((yy & 0x38) << 2);

            // left border
            for (int x = 0; x < (4 * 8); x++)
            {
                *dst++ = sys->border_color;
            }

            // valid 256x192 vidmem area
            for (uint16_t x = 0; x < 32; x++)
            {
                const uint16_t pix_offset = y_offset | x;
                const uint16_t clr_offset = 0x1800 + (((yy & ~0x7) << 2) | x);

                // pixel mask and color attribute bytes
                const uint8_t pix = vidmem_bank[pix_offset];
                const uint8_t clr = vidmem_bank[clr_offset];

                // foreground and background color
                if ((clr & (1 << 7)) && blink)
                {
                    fg = _zx_palette[(clr >> 3) & 7];
                    bg = _zx_palette[clr & 7];
                }
                else
                {
                    fg = _zx_palette[clr & 7];
                    bg = _zx_palette[(clr >> 3) & 7];
                }
                if (0 == (clr & (1 << 6)))
                {
                    // standard brightness
                    fg &= 0xFFD7D7D7;
                    bg &= 0xFFD7D7D7;
                }
                for (int px = 7; px >= 0; px--)
                {
                    *dst++ = pix & (1 << px) ? fg : bg;
                }
            }

            // right border
            for (int x = 0; x < (4 * 8); x++)
            {
                *dst++ = sys->border_color;
            }
        }
    }

    if (sys->scanline_y++ >= sys->frame_scan_lines)
    {
        // start new frame, request vblank interrupt
        sys->scanline_y = 0;
        sys->blink_counter++;
        return true;
    }

    return false;
}
