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
