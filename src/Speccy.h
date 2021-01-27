#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CHIPS_IMPL
#include "Z80.h"
#include "Rom.h"
#include "mem.h"


// clk
typedef struct
{
    int64_t freq_hz;
    int ticks_to_run;
    int overrun_ticks;
} clk_t;

uint32_t clk_us_to_ticks(uint64_t freq_hz, uint32_t micro_seconds);
void clk_init(clk_t* clk, uint32_t freq_hz);
uint32_t clk_ticks_to_run(clk_t* clk, uint32_t micro_seconds);
void clk_ticks_executed(clk_t* clk, uint32_t ticks);

void clk_init(clk_t* clk, uint32_t freq_hz)
{
    CHIPS_ASSERT(clk && (freq_hz > 1));
    memset(clk, 0, sizeof(clk_t));
    clk->freq_hz = freq_hz;
}

uint32_t clk_us_to_ticks(uint64_t freq_hz, uint32_t micro_seconds)
{
    return (uint32_t)((freq_hz * micro_seconds) / 1000000);
}

uint32_t clk_ticks_to_run(clk_t* clk, uint32_t micro_seconds)
{
    CHIPS_ASSERT(clk && (micro_seconds > 0));
    int ticks = (int)((clk->freq_hz * micro_seconds) / 1000000);
    clk->ticks_to_run = ticks - clk->overrun_ticks;
    if (clk->ticks_to_run < 1)
    {
        clk->ticks_to_run = 1;
    }
    return clk->ticks_to_run;
}

void clk_ticks_executed(clk_t* clk, uint32_t ticks_executed)
{
    if ((int)ticks_executed > clk->ticks_to_run)
    {
        clk->overrun_ticks = (int)ticks_executed - clk->ticks_to_run;
    }
    else {
        clk->overrun_ticks = 0;
    }
}

// clk

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (256)
#define DISPLAY_PIXEL_BYTES sizeof(uint32_t)
//#define SYSTEM_MEMORY_BYTES 49152

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

z80_desc_t cpu_desc;

typedef struct
{
    z80_t cpu;
    bool valid;
    bool memory_paging_disabled;
    uint8_t kbd_joymask;        /* joystick mask from keyboard joystick emulation */
    uint8_t joy_joymask;        /* joystick mask from zx_joystick() */
    uint32_t tick_count;
    uint8_t last_mem_config;        /* last out to 0x7FFD */
    uint8_t last_fe_out;            /* last out value to 0xFE port */
    uint8_t blink_counter;          /* incremented on each vblank */
    int frame_scan_lines;
    int top_border_scanlines;
    int scanline_period;
    int scanline_counter;
    int scanline_y;
    uint32_t display_ram_bank;
    uint32_t border_color;
    clk_t clk;
    //kbd_t kbd;
    mem_t mem;
    uint32_t* pixel_buffer;
    void* user_data;
    //zx_audio_callback_t audio_cb;
    int num_samples;
    int sample_pos;
    //float sample_buffer[ZX_MAX_AUDIO_SAMPLES];
    uint8_t ram[8][0x4000];
    uint8_t junk[0x4000];
} zx_t;

void zx_init(zx_t* sys, const zx_desc_t* desc);
void zx_exec(zx_t* sys, uint32_t micro_seconds);
static uint64_t _zx_tick(int num, uint64_t pins, void* user_data);
static bool _zx_decode_scanline(zx_t* sys);
static void _zx_init_memory_map(zx_t* sys);

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

    z80_set_pc(&sys->cpu, 0x0000);
}

void zx_exec(zx_t* sys, uint32_t micro_seconds)
{
    CHIPS_ASSERT(sys && sys->valid);
    uint32_t ticks_to_run = clk_ticks_to_run(&sys->clk, micro_seconds);
    uint32_t ticks_executed = z80_exec(&sys->cpu, ticks_to_run);
    clk_ticks_executed(&sys->clk, ticks_executed);

    //kbd_update(&sys->kbd, micro_seconds);
}

static void _zx_init_memory_map(zx_t* sys)
{
    mem_init(&sys->mem);
    mem_map_ram(&sys->mem, 0, 0x4000, 0x4000, sys->ram[0]);
    mem_map_ram(&sys->mem, 0, 0x8000, 0x4000, sys->ram[1]);
    mem_map_ram(&sys->mem, 0, 0xC000, 0x4000, sys->ram[2]);
    mem_map_rom(&sys->mem, 0, 0x0000, 0x4000, &rom48k[0]);
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

    // tick audio systems
    for (int i = 0; i < num_ticks; i++)
    {
        sys->tick_count++;
        /*bool sample_ready = beeper_tick(&sys->beeper);
        // the AY-3-8912 chip runs at half CPU frequency
        if (sys->type == ZX_TYPE_128)
        {
            if (sys->tick_count & 1)
            {
                ay38910_tick(&sys->ay);
            }
        }
        if (sample_ready)
        {
            float sample = sys->beeper.sample;
            if (sys->type == ZX_TYPE_128) {
                sample += sys->ay.sample;
            }
            sys->sample_buffer[sys->sample_pos++] = sample;
            if (sys->sample_pos == sys->num_samples) {
                if (sys->audio_cb) {
                    sys->audio_cb(sys->sample_buffer, sys->num_samples, sys->user_data);
                }
                sys->sample_pos = 0;
            }
        }*/
    }

    // memory and IO requests
    if (pins & Z80_MREQ)
    {
        // a memory request machine cycle
        //  FIXME: 'contended memory' accesses should inject wait states
        //
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
        // an IO request machine cycle
        //  see http://problemkaputt.de/zxdocs.htm#zxspectrum for address decoding
        //
        if (pins & Z80_RD)
        {
            // an IO read
            //  FIXME: reading from port xxFF should return 'current VRAM data'
            //
            /*if ((pins & Z80_A0) == 0) {
                // Spectrum ULA (...............0)
                //  Bits 5 and 7 as read by INning from Port 0xfe are always one
                //
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
            else if (sys->type == ZX_TYPE_128)
            {
                // read from AY-3-8912 (11............0.)
                if ((pins & (Z80_A15 | Z80_A14 | Z80_A1)) == (Z80_A15 | Z80_A14)) {
                    pins = ay38910_iorq(&sys->ay, AY38910_BC1 | pins) & Z80_PIN_MASK;
                }
            }*/
        }
        else if (pins & Z80_WR)
        {
            // an IO write
            const uint8_t data = Z80_GET_DATA(pins);
            if ((pins & Z80_A0) == 0)
            {
                // Spectrum ULA (...............0)
                //  "every even IO port addresses the ULA but to avoid
                //  problems with other I/O devices, only FE should be used"
                //  FIXME:
                //      bit 3: MIC output (CAS SAVE, 0=On, 1=Off)
                //
                sys->border_color = _zx_palette[data & 7] & 0xFFD7D7D7;
                sys->last_fe_out = data;
                //beeper_set(&sys->beeper, 0 != (data & (1 << 4)));
            }
            /*else if (sys->type == ZX_TYPE_128)
            {
                // Spectrum 128 memory control (0.............0.)
                //   http://8bit.yarek.pl/computer/zx.128/
                //
                if ((pins & (Z80_A15 | Z80_A1)) == 0)
                {
                    if (!sys->memory_paging_disabled)
                    {
                        sys->last_mem_config = data;
                        // bit 3 defines the video scanout memory bank (5 or 7)
                        sys->display_ram_bank = (data & (1 << 3)) ? 7 : 5;
                        // only last memory bank is mappable
                        mem_map_ram(&sys->mem, 0, 0xC000, 0x4000, sys->ram[data & 0x7]);

                        // ROM0 or ROM1
                        if (data & (1 << 4))
                        {
                            // bit 4 set: ROM1
                            mem_map_rom(&sys->mem, 0, 0x0000, 0x4000, sys->rom[1]);
                        }
                        else
                        {
                            // bit 4 clear: ROM0
                            mem_map_rom(&sys->mem, 0, 0x0000, 0x4000, sys->rom[0]);
                        }
                    }
                    if (data & (1 << 5))
                    {
                        // bit 5 prevents further changes to memory pages
                        //  until computer is reset, this is used when switching
                        //  to the 48k ROM
                        //
                        sys->memory_paging_disabled = true;
                    }
                }
                else if ((pins & (Z80_A15 | Z80_A14 | Z80_A1)) == (Z80_A15 | Z80_A14))
                {
                    // select AY-3-8912 register (11............0.)
                    ay38910_iorq(&sys->ay, AY38910_BDIR | AY38910_BC1 | pins);
                }
                else if ((pins & (Z80_A15 | Z80_A14 | Z80_A1)) == Z80_A15)
                {
                    // write to AY-3-8912 (10............0.)
                    ay38910_iorq(&sys->ay, AY38910_BDIR | pins);
                }
            }*/
        }
    }
    return pins;
}

/* this is called by the timer callback for every PAL line, controlling
    the vidmem decoding and vblank interrupt
    detailed information about frame timings is here:
    for 48K:    http://rk.nvg.ntnu.no/sinclair/faq/tech_48.html#48K
    for 128K:   http://rk.nvg.ntnu.no/sinclair/faq/tech_128.html
    one PAL line takes 224 T-states on 48K, and 228 T-states on 128K
    one PAL frame is 312 lines on 48K, and 311 lines on 128K
    decode the next videomem line into the emulator framebuffer,
    the border area of a real Spectrum is bigger than the emulator
    (the emu has 32 pixels border on each side, the hardware has:
    63 or 64 lines top border
    56 border lines bottom border
    48 pixels on each side horizontal border
*/
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
            /* upper/lower border */
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

}
