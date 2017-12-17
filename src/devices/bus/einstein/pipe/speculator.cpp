// license: GPL-2.0+
// copyright-holders: Dirk Best
/***************************************************************************

    Speculator

    TODO: Not working, based on the schematics of the Memotech MTX version

***************************************************************************/

#include "emu.h"
#include "speculator.h"
#include "machine/rescap.h"
#include "sound/wave.h"
#include "formats/tzx_cas.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(EINSTEIN_SPECULATOR, einstein_speculator_device, "einstein_speculator", "Einstein Speculator")

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

MACHINE_CONFIG_MEMBER( einstein_speculator_device::device_add_mconfig )
	MCFG_DEVICE_ADD("ic5a", TTL74123, 0)
	MCFG_TTL74123_CONNECTION_TYPE(TTL74123_NOT_GROUNDED_NO_DIODE)
	MCFG_TTL74123_RESISTOR_VALUE(RES_K(47))
	MCFG_TTL74123_CAPACITOR_VALUE(CAP_P(560))
	MCFG_TTL74123_A_PIN_VALUE(1)
	MCFG_TTL74123_B_PIN_VALUE(1)
	MCFG_TTL74123_CLEAR_PIN_VALUE(0)
	MCFG_TTL74123_OUTPUT_CHANGED_CB(WRITELINE(einstein_speculator_device, ic5a_q_w))

	MCFG_DEVICE_ADD("ic5b", TTL74123, 0)
	MCFG_TTL74123_CONNECTION_TYPE(TTL74123_NOT_GROUNDED_NO_DIODE)
	MCFG_TTL74123_RESISTOR_VALUE(RES_K(47))
	MCFG_TTL74123_CAPACITOR_VALUE(CAP_P(560))
	MCFG_TTL74123_A_PIN_VALUE(1)
	MCFG_TTL74123_B_PIN_VALUE(1)
	MCFG_TTL74123_CLEAR_PIN_VALUE(0)
	MCFG_TTL74123_OUTPUT_CHANGED_CB(WRITELINE(einstein_speculator_device, ic5b_q_w))

	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_CASSETTE_ADD("cassette")
	MCFG_CASSETTE_FORMATS(tzx_cassette_formats)
	MCFG_CASSETTE_DEFAULT_STATE(CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED)
	MCFG_CASSETTE_INTERFACE("spectrum_cass")
MACHINE_CONFIG_END


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  einstein_speculator_device - constructor
//-------------------------------------------------

einstein_speculator_device::einstein_speculator_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, EINSTEIN_SPECULATOR, tag, owner, clock),
	device_tatung_pipe_interface(mconfig, *this),
	m_ic5a(*this, "ic5a"), m_ic5b(*this, "ic5b"),
	m_cassette(*this, "cassette"),
	m_speaker(*this, "speaker"),
	m_nmisel(0)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void einstein_speculator_device::device_start()
{
	// setup ram
	m_ram = std::make_unique<uint8_t[]>(0x800);
	memset(m_ram.get(), 0xff, 0x800);

	// register for save states
	save_pointer(NAME(m_ram.get()), 0x800);
	save_item(NAME(m_nmisel));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void einstein_speculator_device::device_reset()
{
	// ram: range 0x1f, 0x3f, 0x5f, 0x7f, 0x9f, 0xbf, 0xdf, 0xff
	io_space().install_readwrite_handler(0x1f, 0x1f, 0, 0, 0xffe0,
		read8_delegate(FUNC(einstein_speculator_device::ram_r), this),
		write8_delegate(FUNC(einstein_speculator_device::ram_w), this));

	// ram: range 0x60 - 0xff
	io_space().install_readwrite_handler(0x60, 0x60, 0, 0, 0xff9f,
		read8_delegate(FUNC(einstein_speculator_device::ram_r), this),
		write8_delegate(FUNC(einstein_speculator_device::ram_w), this));

	// tape read/nmi write register: range 0xff
	io_space().install_readwrite_handler(0xff, 0xff, 0, 0, 0xff00,
		read8_delegate(FUNC(einstein_speculator_device::tape_r), this),
		write8_delegate(FUNC(einstein_speculator_device::nmi_w), this));
}


//**************************************************************************
//  IMPLEMENTATION
//**************************************************************************

WRITE_LINE_MEMBER( einstein_speculator_device::ic5a_q_w )
{
	m_ic5b->a_w(state);

	if (m_nmisel == 1)
	{
		logerror("imm nmi: %d\n", state);
		m_slot->nmi_w(state);
	}
}

WRITE_LINE_MEMBER( einstein_speculator_device::ic5b_q_w )
{
	if (m_nmisel == 0)
	{
		logerror("del nmi: %d\n", state);
		m_slot->nmi_w(state);
	}
}

void einstein_speculator_device::int_w(int state)
{
	logerror("int_w: %d\n", state);
	m_ic5a->a_w(!state);
}

// pal ic4
offs_t einstein_speculator_device::address_translate(offs_t offset)
{
	int ra3, ra2, ra1, ra0;

	ra3 = !BIT(offset, 7) && BIT(offset, 0);

	ra2  = BIT(offset, 15) && BIT(offset, 14) && BIT(offset, 13) && BIT(offset, 12) && !BIT(offset, 11);
	ra2 |= BIT(offset, 15) && BIT(offset, 14) && BIT(offset, 13) && BIT(offset, 12) && !BIT(offset, 10);
	ra2 |= BIT(offset, 15) && BIT(offset, 14) && BIT(offset, 13) && BIT(offset, 12) && !BIT(offset, 9);
	ra2 |= BIT(offset, 15) && BIT(offset, 14) && BIT(offset, 13) && BIT(offset, 12) && !BIT(offset, 8);

	ra1  = BIT(offset, 15) && BIT(offset, 14) && !BIT(offset, 13);
	ra1 |= BIT(offset, 15) && BIT(offset, 14) && !BIT(offset, 12);
	ra1 |= BIT(offset, 15) && BIT(offset, 14) &&  BIT(offset, 11) && BIT(offset, 10) && !BIT(offset, 9);
	ra1 |= BIT(offset, 15) && BIT(offset, 14) &&  BIT(offset, 11) && BIT(offset, 10) && !BIT(offset, 8);

	ra0  = BIT(offset, 15) && !BIT(offset, 14);
	ra0 |= BIT(offset, 15) &&  BIT(offset, 13) && !BIT(offset, 12);
	ra0 |= BIT(offset, 15) &&  BIT(offset, 13) &&  BIT(offset, 11) && !BIT(offset, 10);
	ra0 |= BIT(offset, 15) &&  BIT(offset, 13) &&  BIT(offset, 11) &&  BIT(offset,  9) && !BIT(offset, 8);

	return (ra3 << 3) | (ra2 << 2) | (ra1 << 1) | (ra0 << 0);
}

READ8_MEMBER( einstein_speculator_device::ram_r )
{
	offs_t addr = ((offset << 4) & 0x7f) | address_translate(offset);
	return m_ram[addr];
}

WRITE8_MEMBER( einstein_speculator_device::ram_w )
{
	offs_t addr = ((offset << 4) & 0x7f) | address_translate(offset);
	m_ram[addr] = data;
}

READ8_MEMBER( einstein_speculator_device::tape_r )
{
	// 7654321-  unknown
	// -------0  cassette input

	return m_cassette->input() > 0.0038 ? 1 : 0;
}

WRITE8_MEMBER( einstein_speculator_device::nmi_w )
{
	logerror("nmi_w offset %04x data %02x\n", offset, data);

	// 76543---  unknown
	// -----2--  nmi enable?
	// ------1-  nmi select?
	// -------0  speaker?

	m_ic5a->clear_w(BIT(data, 2));
	m_ic5b->clear_w(BIT(data, 2));

	m_nmisel = BIT(data, 1);

	m_speaker->level_w(BIT(data, 0));
}