#include "board.h"
#include <inttypes.h>
#include "adc.h"
#include "esp_log.h"
#include "defs.h"
#include "fpga.h"

/**
 * \ingroup init
 * \defgroup board Board routines
 * \brief Board routines
 *
 * These routines allow for identifcation of the physical board where the firmware
 * is running.
 */

/*
 * Boards in production
 *
 * 2.2p1
 * 2.3
 * 2.4   Compatible with 2.3
 */

#define VOLTAGE_DELTA_MV 1300

static volatile uint32_t supply_rail_voltage;

#define RESISTOR_R1_HUNDREDOHMS 130 /* 13000 ohm */
#define RESISTOR_R2_HUNDREDOHMS 390 /* 39000 ohm  - r2.4 */

/**
 \ingroup board
 \brief Initialise the board

 This should be called only once at system startup
 */
void board__init()
{
    uint32_t adcval = adc__read_v();
    adcval *= RESISTOR_R1_HUNDREDOHMS + RESISTOR_R2_HUNDREDOHMS;
    adcval /=  RESISTOR_R1_HUNDREDOHMS;
    ESP_LOGI(TAG, "Supply voltage rail: %dmV", adcval);
    supply_rail_voltage = adcval;
}

static bool board__inrail(unsigned mv)
{
    ESP_LOGI(TAG, "Check rails %d : low %d high %d",
             supply_rail_voltage,
             mv-VOLTAGE_DELTA_MV,
             mv+VOLTAGE_DELTA_MV);
    return ( (supply_rail_voltage >= (mv-VOLTAGE_DELTA_MV)) &&
            (supply_rail_voltage <= (mv+VOLTAGE_DELTA_MV)));
}

/**
 \ingroup board
 \brief Check if board is running with 12V supply

 \return true if the voltage is within limits for 12V operation
 */
bool board__is12Vsupply(void)
{
    return supply_rail_voltage > 11200;
}

/**
 \ingroup board
 \brief Check if board is running with 5V supply

 \return true if the voltage is within limits for 5V operation
 */
bool board__is5Vsupply(void)
{
    return board__inrail(5000);
}

/**
 \ingroup board
 \brief Check if board is running with 9V supply

 \return true if the voltage is within limits for 9V operation
 */
bool board__is9Vsupply(void)
{
    return board__inrail(9000);
}

/**
 \ingroup board
 \brief Check if board has voltage sensor.

 All boards starting with 2.3 include a voltage sensor.

 \return true if the board includes a voltage sensor
 */

bool board__hasVoltageSensor(void)
{
    return supply_rail_voltage > 3000;
}

/**
 \ingroup board
 \brief Check if board is compatible with a certain version

 This is used for firmware upgrade compatibility.

 For example, calling board__isCompatible(2,4) will return true if the current hardware
 can be used for a 2.4 firmware.

 \param major Major board version to check
 \param minor Minor board version to check
 \return true if the board matches the required minor/major versions
 */
bool board__isCompatible(uint8_t major, uint8_t minor)
{
    if (major!=2)
        return false;

    switch (minor) {
    case 2:
        return !board__hasVoltageSensor();
        break;
    case 3: /* Fall-through */
    case 4:
        return board__hasVoltageSensor();
        break;
    default:
        return false;
    }
}

