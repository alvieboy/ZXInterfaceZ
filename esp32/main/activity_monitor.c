/**
 * \defgroup activity_monitor Activity monitor
 * \brief Miscelaneous activity monitors
 *
 * This module reports some of the activity monitor values.
 */
#include "activity_monitor.h"
#include "fpga.h"
/**
 * \ingroup activity_monitor
 * \brief Read MIC idle
 *
 * Read the MIC idle from the FPGA, i.e., the number of milisseconds since there was no
 * activity on the ULA mic line.
 *
 * \return The number of milliseconds elapsed since last transition was detected on the
 * MIC line.
 */
int activity_monitor__read_mic_idle()
{
    return fpga__read_mic_idle();
}

