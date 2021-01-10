#include "board.h"
#include <inttypes.h>
#include "adc.h"
#include "esp_log.h"
#include "defs.h"
#include "fpga.h"

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
//#define RESISTOR_R2_HUNDREDOHMS 301 /* 30100 ohm  - old r2.3 prototype */
#define RESISTOR_R2_HUNDREDOHMS 390 /* 39000 ohm  - r2.4 */


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

bool board__is12Vsupply(void)
{
    return supply_rail_voltage > 11200;
}

bool board__is5Vsupply(void)
{
    return board__inrail(5000);
}

bool board__is9Vsupply(void)
{
    return board__inrail(9000);
}

bool board__hasVoltageSensor(void)
{
    return supply_rail_voltage > 3000;
}

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

