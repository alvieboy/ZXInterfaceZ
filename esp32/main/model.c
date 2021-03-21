#include "model.h"

static model_t model = MODEL_UNKNOWN;

model_t model__get(void)
{
    return model;
}
void model__set(model_t m)
{
    model = m;
}

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
