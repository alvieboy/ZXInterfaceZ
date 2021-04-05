/**
 * \defgroup model
 * \brief ZX Spectrum models
 */
#include "model.h"

static model_t model = MODEL_UNKNOWN;

/**
 * \ingroup model
 * \brief Return the running ZX Spectrum model
 */
model_t model__get(void)
{
    return model;
}

/**
 * \ingroup model
 * \brief Set the running ZX Spectrum model
 */
void model__set(model_t m)
{
    model = m;
}

/**
 * \ingroup model
 * \brief Check if current model supports ULA override (i.e., IORQULA signal)
 */
bool model__supports_ula_override()
{
    bool support = false;
    switch(model) {
    case MODEL_16K:
    case MODEL_48K:
        support = true;
        break;
    case MODEL_128K:
        // TODO: Check for toastrack/investronica.
        support = true;
        break;
    default:
        break;
    }
    return support;
}

int model__get_basic_rom()
{
    int rom = 1;
    switch(model) {
    case MODEL_16K:
    case MODEL_48K:
        rom = 0;
        break;
    default:
        break;
    }
    return rom;
}
