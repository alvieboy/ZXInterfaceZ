#include "board.h"
#include <inttypes.h>
#include "adc.h"
#include "esp_log.h"
#include "defs.h"

/*
 * Boards in production
 *
 * 2.2p1
 * 2.3
 * 2.4   Compatible with 2.3
 */

#define VOLTAGE_DELTA_MV 800

static volatile uint32_t supply_rail_voltage;

void board__init()
{
    uint32_t adcval = adc__read_v();
    adcval *= 431;
    adcval /=  130;
    ESP_LOGI(TAG, "Supply voltage rail: %dmV", adcval);
    supply_rail_voltage = adcval;
}

static bool board__inrail(unsigned mv)
{
    return ( (supply_rail_voltage >= (mv-VOLTAGE_DELTA_MV)) &&
            (supply_rail_voltage <= (mv+VOLTAGE_DELTA_MV)));
}

bool board__is5Vsupply(void)
{
    return board__inrail(5000);
}

bool board__is9Vsupply(void)
{
    return board__inrail(9000);
}

static bool board__hasVoltageSensor()
{
    return board__is5Vsupply() || board__is9Vsupply();
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
