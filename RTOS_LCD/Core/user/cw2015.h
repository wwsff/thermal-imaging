#ifndef CW2015_H
#define CW2015_H

#include <stdint.h>

typedef struct
{
    uint8_t version;
    uint16_t soc_x10;
    uint16_t voltage_mv;
} cw2015_data_t;

int cw2015_read(cw2015_data_t *data);

#endif
