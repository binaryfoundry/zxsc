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

const int cpu_freq = 3500000;

const static uint32_t _zx_palette[8] =
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

static const char* _zx_keymap =
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
    uint8_t junk[0x4000];
} zx_t;

static void zx_init(zx_t* sys, const zx_desc_t* desc);
static void zx_exec(zx_t* sys, uint32_t micro_seconds);
static bool zx_quickload(zx_t* sys, const uint8_t* ptr, int num_bytes);
static void zx_key_down(zx_t* sys, int key_code);
static void zx_key_up(zx_t* sys, int key_code);

static uint64_t _zx_tick(int num, uint64_t pins, void* user_data);
static bool _zx_decode_scanline(zx_t* sys);
static void _zx_init_memory_map(zx_t* sys);
static void _zx_init_keyboard_matrix(zx_t* sys);

#define _ZX_DEFAULT(val,def) (((val) != 0) ? (val) : (def));
#define _ZX_CLEAR(val) memset(&val, 0, sizeof(val))

static void zx_init(zx_t* sys, const zx_desc_t* desc)
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

static void zx_exec(zx_t* sys, uint32_t micro_seconds)
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

static void zx_key_down(zx_t* sys, int key_code)
{
    CHIPS_ASSERT(sys && sys->valid);
    kbd_key_down(&sys->kbd, key_code);
}

static void zx_key_up(zx_t* sys, int key_code)
{
    CHIPS_ASSERT(sys && sys->valid);
    kbd_key_up(&sys->kbd, key_code);
}

static void _zx_init_keyboard_matrix(zx_t* sys)
{
    kbd_init(&sys->kbd, 1);
    kbd_register_modifier(&sys->kbd, 0, 0, 0);
    kbd_register_modifier(&sys->kbd, 1, 7, 1);

    for (int layer = 0; layer < 3; layer++)
    {
        for (int column = 0; column < 8; column++)
        {
            for (int line = 0; line < 5; line++)
            {
                const uint8_t c = _zx_keymap[layer * 40 + column * 5 + line];
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
                //Z80_SET_DATA(pins, sys->kbd_joymask | sys->joy_joymask);
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

// ZX Z80 file format header (http://www.worldofspectrum.org/faq/reference/z80format.htm )
typedef struct
{
    uint8_t A, F;
    uint8_t C, B;
    uint8_t L, H;
    uint8_t PC_l, PC_h;
    uint8_t SP_l, SP_h;
    uint8_t I, R;
    uint8_t flags0;
    uint8_t E, D;
    uint8_t C_, B_;
    uint8_t E_, D_;
    uint8_t L_, H_;
    uint8_t A_, F_;
    uint8_t IY_l, IY_h;
    uint8_t IX_l, IX_h;
    uint8_t EI;
    uint8_t IFF2;
    uint8_t flags1;
} _zx_z80_header;

typedef struct
{
    uint8_t len_l;
    uint8_t len_h;
    uint8_t PC_l, PC_h;
    uint8_t hw_mode;
    uint8_t out_7ffd;
    uint8_t rom1;
    uint8_t flags;
    uint8_t out_fffd;
    uint8_t audio[16];
    uint8_t tlow_l;
    uint8_t tlow_h;
    uint8_t spectator_flags;
    uint8_t mgt_rom_paged;
    uint8_t multiface_rom_paged;
    uint8_t rom_0000_1fff;
    uint8_t rom_2000_3fff;
    uint8_t joy_mapping[10];
    uint8_t kbd_mapping[10];
    uint8_t mgt_type;
    uint8_t disciple_button_state;
    uint8_t disciple_flags;
    uint8_t out_1ffd;
} _zx_z80_ext_header;

typedef struct
{
    uint8_t len_l;
    uint8_t len_h;
    uint8_t page_nr;
} _zx_z80_page_header;

static bool _zx_overflow(const uint8_t* ptr, intptr_t num_bytes, const uint8_t* end_ptr)
{
    return (ptr + num_bytes) > end_ptr;
}

static bool zx_quickload(zx_t* sys, const uint8_t* ptr, int num_bytes)
{
    const uint8_t* end_ptr = ptr + num_bytes;
    if (_zx_overflow(ptr, sizeof(_zx_z80_header), end_ptr))
    {
        return false;
    }
    const _zx_z80_header* hdr = (const _zx_z80_header*)ptr;
    ptr += sizeof(_zx_z80_header);
    const _zx_z80_ext_header* ext_hdr = 0;
    uint16_t pc = (hdr->PC_h << 8 | hdr->PC_l) & 0xFFFF;
    const bool is_version1 = 0 != pc;
    if (!is_version1)
    {
        if (_zx_overflow(ptr, sizeof(_zx_z80_ext_header), end_ptr))
        {
            return false;
        }
        ext_hdr = (_zx_z80_ext_header*)ptr;
        int ext_hdr_len = (ext_hdr->len_h << 8) | ext_hdr->len_l;
        ptr += 2 + ext_hdr_len;
        if (ext_hdr->hw_mode < 3)
        {
        }
        else
        {
            return false;
        }
    }
    else
    {
    }
    const bool v1_compr = 0 != (hdr->flags0 & (1 << 5));
    while (ptr < end_ptr)
    {
        int page_index = 0;
        int src_len = 0;
        if (is_version1)
        {
            src_len = num_bytes - sizeof(_zx_z80_header);
        }
        else
        {
            _zx_z80_page_header* phdr = (_zx_z80_page_header*)ptr;
            if (_zx_overflow(ptr, sizeof(_zx_z80_page_header), end_ptr))
            {
                return false;
            }
            ptr += sizeof(_zx_z80_page_header);
            src_len = (phdr->len_h << 8 | phdr->len_l) & 0xFFFF;
            page_index = phdr->page_nr - 3;
            if (page_index == 5)
            {
                page_index = 0;
            }
            if ((page_index < 0) || (page_index > 7))
            {
                page_index = -1;
            }
        }
        uint8_t* dst_ptr;
        if (-1 == page_index)
        {
            dst_ptr = sys->junk;
        }
        else
        {
            dst_ptr = sys->ram[page_index];
        }
        if (0xFFFF == src_len)
        {
            // FIXME: uncompressed not supported yet
            return false;
        }
        else
        {
            // compressed
            int src_pos = 0;
            bool v1_done = false;
            uint8_t val[4];
            while ((src_pos < src_len) && !v1_done)
            {
                val[0] = ptr[src_pos];
                val[1] = ptr[src_pos + 1];
                val[2] = ptr[src_pos + 2];
                val[3] = ptr[src_pos + 3];
                // check for version 1 end marker
                if (v1_compr && (0 == val[0]) && (0xED == val[1]) && (0xED == val[2]) && (0 == val[3]))
                {
                    v1_done = true;
                    src_pos += 4;
                }
                else if (0xED == val[0])
                {
                    if (0xED == val[1])
                    {
                        uint8_t count = val[2];
                        CHIPS_ASSERT(0 != count);
                        uint8_t data = val[3];
                        src_pos += 4;
                        for (int i = 0; i < count; i++) {
                            *dst_ptr++ = data;
                        }
                    }
                    else
                    {
                        *dst_ptr++ = val[0];
                        src_pos++;
                    }
                }
                else
                {
                    *dst_ptr++ = val[0];
                    src_pos++;
                }
            }
            CHIPS_ASSERT(src_pos == src_len);
        }
        if (0xFFFF == src_len)
        {
            ptr += 0x4000;
        }
        else
        {
            ptr += src_len;
        }
    }

    // start loaded image
    z80_reset(&sys->cpu);
    z80_set_a(&sys->cpu, hdr->A); z80_set_f(&sys->cpu, hdr->F);
    z80_set_b(&sys->cpu, hdr->B); z80_set_c(&sys->cpu, hdr->C);
    z80_set_d(&sys->cpu, hdr->D); z80_set_e(&sys->cpu, hdr->E);
    z80_set_h(&sys->cpu, hdr->H); z80_set_l(&sys->cpu, hdr->L);
    z80_set_ix(&sys->cpu, hdr->IX_h << 8 | hdr->IX_l);
    z80_set_iy(&sys->cpu, hdr->IY_h << 8 | hdr->IY_l);
    z80_set_af_(&sys->cpu, hdr->A_ << 8 | hdr->F_);
    z80_set_bc_(&sys->cpu, hdr->B_ << 8 | hdr->C_);
    z80_set_de_(&sys->cpu, hdr->D_ << 8 | hdr->E_);
    z80_set_hl_(&sys->cpu, hdr->H_ << 8 | hdr->L_);
    z80_set_sp(&sys->cpu, hdr->SP_h << 8 | hdr->SP_l);
    z80_set_i(&sys->cpu, hdr->I);
    z80_set_r(&sys->cpu, (hdr->R & 0x7F) | ((hdr->flags0 & 1) << 7));
    z80_set_iff2(&sys->cpu, hdr->IFF2 != 0);
    z80_set_ei_pending(&sys->cpu, hdr->EI != 0);
    if (hdr->flags1 != 0xFF)
    {
        z80_set_im(&sys->cpu, hdr->flags1 & 3);
    }
    else
    {
        z80_set_im(&sys->cpu, 1);
    }
    if (ext_hdr)
    {
        z80_set_pc(&sys->cpu, ext_hdr->PC_h << 8 | ext_hdr->PC_l);
        // simulate an out of port 0xFFFD and 0x7FFD
        uint64_t pins = Z80_IORQ | Z80_WR;
        Z80_SET_ADDR(pins, 0xFFFD);
        Z80_SET_DATA(pins, ext_hdr->out_fffd);
        _zx_tick(4, pins, sys);
        Z80_SET_ADDR(pins, 0x7FFD);
        Z80_SET_DATA(pins, ext_hdr->out_7ffd);
        _zx_tick(4, pins, sys);
    }
    else
    {
        z80_set_pc(&sys->cpu, hdr->PC_h << 8 | hdr->PC_l);
    }
    sys->border_color = _zx_palette[(hdr->flags0 >> 1) & 7] & 0xFFD7D7D7;
    return true;
}
