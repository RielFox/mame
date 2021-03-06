// license:BSD-3-Clause
// copyright-holders:Bryan McPhail,Ernesto Corvi,Andrew Prime,Zsolt Vasvari
// thanks-to:Fuzz
/*************************************************************************

    Neo-Geo hardware

*************************************************************************/

#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "sound/2610intf.h"
#include "machine/gen_latch.h"
#include "machine/upd1990a.h"
#include "machine/ng_memcard.h"
#include "video/neogeo_spr.h"

#include "bus/neogeo/slot.h"
#include "bus/neogeo/carts.h"
#include "bus/neogeo_ctrl/ctrl.h"

#include "screen.h"


// On scanline 224, /VBLANK goes low 56 mclks (14 pixels) from the rising edge of /HSYNC.
// Two mclks after /VBLANK goes low, the hardware sets a pending IRQ1 flip-flop.
#define NEOGEO_VBLANK_IRQ_HTIM (attotime::from_ticks(56+2, NEOGEO_MASTER_CLOCK))


class neogeo_state : public driver_device
{
public:
	neogeo_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu"),
		m_upd4990a(*this, "upd4990a"),
		m_ym(*this, "ymsnd"),
		m_sprgen(*this, "spritegen"),
		m_save_ram(*this, "saveram"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_memcard(*this, "memcard"),
		m_soundlatch(*this, "soundlatch"),
		m_soundlatch2(*this, "soundlatch2"),
		m_region_maincpu(*this, "maincpu"),
		m_region_sprites(*this, "sprites"),
		m_region_fixed(*this, "fixed"),
		m_region_fixedbios(*this, "fixedbios"),
		m_region_mainbios(*this, "mainbios"),
		m_region_audiobios(*this, "audiobios"),
		m_region_audiocpu(*this, "audiocpu"),
		m_bank_audio_main(*this, "audio_main"),
		m_dsw(*this, "DSW"),
		m_trackx(*this, "TRACK_X"),
		m_tracky(*this, "TRACK_Y"),
		m_edge(*this, "edge"),
		m_ctrl1(*this, "ctrl1"),
		m_ctrl2(*this, "ctrl2"),
		m_use_cart_vectors(0),
		m_use_cart_audio(0),
		m_slots(*this, "cslot%u", 1U),
		m_digits(*this, "digit%u", 1U),
		m_lamps(*this, "lamp%u", 1U)
	{ }

	DECLARE_READ16_MEMBER(memcard_r);
	DECLARE_WRITE16_MEMBER(memcard_w);
	DECLARE_WRITE8_MEMBER(audio_command_w);
	DECLARE_READ8_MEMBER(audio_command_r);
	DECLARE_READ8_MEMBER(audio_cpu_bank_select_r);
	DECLARE_WRITE8_MEMBER(audio_cpu_enable_nmi_w);
	DECLARE_READ16_MEMBER(unmapped_r);
	DECLARE_READ16_MEMBER(paletteram_r);
	DECLARE_WRITE16_MEMBER(paletteram_w);
	DECLARE_READ16_MEMBER(video_register_r);
	DECLARE_WRITE16_MEMBER(video_register_w);
	DECLARE_READ16_MEMBER(in0_r);
	DECLARE_READ16_MEMBER(in1_r);

	DECLARE_CUSTOM_INPUT_MEMBER(get_memcard_status);
	DECLARE_CUSTOM_INPUT_MEMBER(get_audio_result);

	TIMER_CALLBACK_MEMBER(display_position_interrupt_callback);
	TIMER_CALLBACK_MEMBER(display_position_vblank_callback);
	TIMER_CALLBACK_MEMBER(vblank_interrupt_callback);

	// MVS-specific
	DECLARE_WRITE_LINE_MEMBER(set_save_ram_unlock);
	DECLARE_WRITE16_MEMBER(save_ram_w);
	DECLARE_CUSTOM_INPUT_MEMBER(kizuna4p_start_r);

	uint32_t screen_update_neogeo(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	DECLARE_WRITE8_MEMBER(io_control_w);
	DECLARE_WRITE_LINE_MEMBER(set_use_cart_vectors);
	DECLARE_WRITE_LINE_MEMBER(set_use_cart_audio);
	DECLARE_READ16_MEMBER(banked_vectors_r);
	DECLARE_WRITE16_MEMBER(write_banksel);
	DECLARE_WRITE16_MEMBER(write_bankprot);
	DECLARE_WRITE16_MEMBER(write_bankprot_pvc);
	DECLARE_WRITE16_MEMBER(write_bankprot_ms5p);
	DECLARE_WRITE16_MEMBER(write_bankprot_kf2k3bl);
	DECLARE_WRITE16_MEMBER(write_bankprot_kof10th);
	DECLARE_READ16_MEMBER(read_lorom_kof10th);

	DECLARE_WRITE_LINE_MEMBER(set_screen_shadow);
	DECLARE_WRITE_LINE_MEMBER(set_palette_bank);

	DECLARE_DRIVER_INIT(neogeo);

	void neogeo_base(machine_config &config);
	void neogeo_arcade(machine_config &config);
	void mslug3b6(machine_config &config);
	void s1945p(machine_config &config);
	void rotd(machine_config &config);
	void mslug4(machine_config &config);
	void kof2003(machine_config &config);
	void lans2004(machine_config &config);
	void ms5plus(machine_config &config);
	void kof2k4se(machine_config &config);
	void kof2002(machine_config &config);
	void kof2001(machine_config &config);
	void svcplus(machine_config &config);
	void kf2k5uni(machine_config &config);
	void garou(machine_config &config);
	void sbp(machine_config &config);
	void cthd2k3(machine_config &config);
	void ct2k3sp(machine_config &config);
	void kof2000n(machine_config &config);
	void mslug3(machine_config &config);
	void kog(machine_config &config);
	void fatfur2(machine_config &config);
	void garouh(machine_config &config);
	void ganryu(machine_config &config);
	void zupapa(machine_config &config);
	void neobase(machine_config &config);
	void kof10th(machine_config &config);
	void mslug5(machine_config &config);
	void garoubl(machine_config &config);
	void ct2k3sa(machine_config &config);
	void nitd(machine_config &config);
	void samsh5sp(machine_config &config);
	void kof98(machine_config &config);
	void kf2k3pl(machine_config &config);
	void mvs(machine_config &config);
	void ms4plus(machine_config &config);
	void samsho5b(machine_config &config);
	void popbounc(machine_config &config);
	void svcplusa(machine_config &config);
	void kof2002b(machine_config &config);
	void svcboot(machine_config &config);
	void kof2000(machine_config &config);
	void samsho5(machine_config &config);
	void kf2k2mp2(machine_config &config);
	void pnyaa(machine_config &config);
	void mslug3h(machine_config &config);
	void vliner(machine_config &config);
	void jockeygp(machine_config &config);
	void matrim(machine_config &config);
	void matrimbl(machine_config &config);
	void kof97oro(machine_config &config);
	void kizuna4p(machine_config &config);
	void mslugx(machine_config &config);
	void kf2k2pls(machine_config &config);
	void kf10thep(machine_config &config);
	void sengoku3(machine_config &config);
	void neogeo_mj(machine_config &config);
	void kf2k3upl(machine_config &config);
	void preisle2(machine_config &config);
	void svcsplus(machine_config &config);
	void kf2k3bl(machine_config &config);
	void kof99(machine_config &config);
	void svc(machine_config &config);
	void kof2003h(machine_config &config);
	void kof99k(machine_config &config);
	void irrmaze(machine_config &config);
	void kf2k2mp(machine_config &config);
	void bangbead(machine_config &config);
	void audio_io_map(address_map &map);
	void audio_map(address_map &map);
	void main_map_slot(address_map &map);
	void neogeo_main_map(address_map &map);
protected:
	void common_machine_start();

	void set_outputs();

	// device overrides
	virtual void machine_start() override;
	virtual void machine_reset() override;

	void neogeo_postload();

	// devices
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	// MVS-specific devices
	optional_device<upd4990a_device> m_upd4990a;
	optional_device<ym2610_device> m_ym;
	required_device<neosprite_optimized_device> m_sprgen;
	optional_shared_ptr<uint16_t> m_save_ram;

	required_device<screen_device> m_screen;
	optional_device<palette_device> m_palette;
	optional_device<ng_memcard_device> m_memcard;
	required_device<generic_latch_8_device> m_soundlatch;
	required_device<generic_latch_8_device> m_soundlatch2;

	// memory
	optional_memory_region m_region_maincpu;
	optional_memory_region m_region_sprites;
	optional_memory_region m_region_fixed;
	optional_memory_region m_region_fixedbios;
	optional_memory_region m_region_mainbios;
	optional_memory_region m_region_audiobios;
	optional_memory_region m_region_audiocpu;
	optional_memory_bank   m_bank_audio_main; // optional because of neocd
	memory_bank           *m_bank_audio_cart[4];
	memory_bank           *m_bank_cartridge;

	// configuration
	enum {NEOGEO_MVS, NEOGEO_AES, NEOGEO_CD} m_type;

	optional_ioport m_dsw;
	optional_ioport m_trackx;
	optional_ioport m_tracky;
	optional_device<neogeo_ctrl_edge_port_device> m_edge;
	optional_device<neogeo_control_port_device> m_ctrl1;
	optional_device<neogeo_control_port_device> m_ctrl2;

	// video hardware, including maincpu interrupts
	// TODO: make into a device
	virtual void video_start() override;
	virtual void video_reset() override;

	const pen_t *m_bg_pen;
	uint8_t      m_vblank_level;
	uint8_t      m_raster_level;

	int m_use_cart_vectors;
	int m_use_cart_audio;

	void set_slot_idx(int slot);

	// cart slots
	void init_cpu();
	void init_audio();
	void init_ym();
	void init_sprites();
	// temporary helper to restore memory banking while bankswitch is handled in the driver...
	uint32_t m_bank_base;

	optional_device_array<neogeo_cart_slot_device, 6> m_slots;
	output_finder<4> m_digits;
	output_finder<6> m_lamps;

	int m_curr_slot;

private:
	void update_interrupts();
	void create_interrupt_timers();
	void start_interrupt_timers();
	void acknowledge_interrupt(uint16_t data);

	void adjust_display_position_interrupt_timer();
	void set_display_position_interrupt_control(uint16_t data);
	void set_display_counter_msb(uint16_t data);
	void set_display_counter_lsb(uint16_t data);
	void set_video_control(uint16_t data);

	void create_rgb_lookups();
	void set_pens();

	void audio_cpu_check_nmi();
	void set_output_latch(uint8_t data);
	void set_output_data(uint8_t data);

	// internal state
	bool       m_recurse;
	bool       m_audio_cpu_nmi_enabled;
	bool       m_audio_cpu_nmi_pending;

	// MVS-specific state
	uint8_t      m_save_ram_unlocked;
	uint8_t      m_output_data;
	uint8_t      m_output_latch;
	uint8_t      m_el_value;
	uint8_t      m_led1_value;
	uint8_t      m_led2_value;

	emu_timer  *m_display_position_interrupt_timer;
	emu_timer  *m_display_position_vblank_timer;
	emu_timer  *m_vblank_interrupt_timer;
	uint32_t     m_display_counter;
	uint8_t      m_vblank_interrupt_pending;
	uint8_t      m_display_position_interrupt_pending;
	uint8_t      m_irq3_pending;
	uint8_t      m_display_position_interrupt_control;

	uint16_t get_video_control();

	// color/palette related
	std::vector<uint16_t> m_paletteram;
	uint8_t        m_palette_lookup[32][4];
	int          m_screen_shadow;
	int          m_palette_bank;
};


class aes_state : public neogeo_state
{
	public:
		aes_state(const machine_config &mconfig, device_type type, const char *tag)
			: neogeo_state(mconfig, type, tag)
			, m_io_in2(*this, "IN2")
	{}

	DECLARE_READ16_MEMBER(aes_in2_r);
	DECLARE_INPUT_CHANGED_MEMBER(aes_jp1);
	DECLARE_MACHINE_START(aes);

	void aes(machine_config &config);
	void aes_main_map(address_map &map);
protected:
	required_ioport m_io_in2;
};


#include "bus/neogeo/prot_pcm2.h"
#include "bus/neogeo/prot_cmc.h"
#include "bus/neogeo/prot_pvc.h"

class neopcb_state : public neogeo_state
{
	public:
		neopcb_state(const machine_config &mconfig, device_type type, const char *tag)
			: neogeo_state(mconfig, type, tag)
		, m_cmc_prot(*this, "cmc50")
		, m_pcm2_prot(*this, "pcm2")
		, m_pvc_prot(*this, "pvc")
	{}

	// device overrides
	virtual void machine_start() override;

	DECLARE_WRITE16_MEMBER(write_bankpvc);

	DECLARE_INPUT_CHANGED_MEMBER(select_bios);

	DECLARE_DRIVER_INIT(ms5pcb);
	DECLARE_DRIVER_INIT(svcpcb);
	DECLARE_DRIVER_INIT(kf2k3pcb);
	DECLARE_DRIVER_INIT(vliner);

	void install_common();
	void install_banked_bios();
	void neopcb_postload();
	// non-carts
	void svcpcb_gfx_decrypt();
	void svcpcb_s1data_decrypt();
	void kf2k3pcb_gfx_decrypt();
	void kf2k3pcb_decrypt_s1data();
	void kf2k3pcb_sp1_decrypt();

	required_device<cmc_prot_device> m_cmc_prot;
	required_device<pcm2_prot_device> m_pcm2_prot;
	required_device<pvc_prot_device> m_pvc_prot;
	void neopcb(machine_config &config);
};


/*----------- defined in drivers/neogeo.c -----------*/

INPUT_PORTS_EXTERN(neogeo);
INPUT_PORTS_EXTERN(aes);
