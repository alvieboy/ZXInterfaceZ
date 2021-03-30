/**
 * \defgroup kempston
 * \brief Kempston interfaces
 */
#include "kempston.h"
#include "fpga.h"
#include "log.h"

static uint32_t kempston_regs = 0x000000;

/**
 * \ingroup kempston
 * \brief Intialise the kempston interfaces
 */
void kempston__init()
{
    kempston_regs = 0x000000;
    fpga__set_register(REG_KEMPSTON, kempston_regs);

    // Enable joystick
    fpga__set_config1_bits(CONFIG1_JOY_ENABLE);
}

/**
 * \ingroup kempston
 * \brief Set the Kempston mouse values
 */
void kempston__set_mouse(uint8_t x, uint8_t y, uint8_t button1, uint8_t button2)
{
    uint32_t newval = kempston_regs;
    newval &= ~0x3FFFF; // Clear x,y, buttons
    newval |= x;
    newval |= (uint32_t)y <<8;
    if (button1)
        newval |= (1<<16);
    if (button2)
        newval |= (1<<17);

    kempston_regs = newval;
    fpga__set_register(REG_KEMPSTON, kempston_regs);
}

/**
 * \ingroup kempston
 * \brief Set the Kempston joystick raw value, as read from Spectum port $1F
 */
void kempston__set_joystick_raw(uint8_t val)
{
    uint32_t newval = kempston_regs;

    newval &= ~0xFC0000; // Clear joystick
    newval &= 0x7F;
    newval |= ((uint32_t)val) << 18;

    kempston_regs = newval;
    fpga__set_register(REG_KEMPSTON, kempston_regs);
}

/**
 * \ingroup kempston
 * \brief Set the Kempston joystick axix/button
 */
void kempston__set_joystick(joy_action_t axis, bool on)
{
    uint32_t newval = kempston_regs;
    if (on) {
        newval |= (1<<((int)axis+18));
    } else {
        newval &= ~(1<<((int)axis+18));
    }
//    ESP_LOGI("KEMPSTON", "Updating reg %08x\n", newval & 0x3F);
    fpga__set_register(REG_KEMPSTON, newval);
    kempston_regs = newval;
}

