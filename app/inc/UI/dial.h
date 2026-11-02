#ifndef DIAL_H
#define DIAL_H

#include <stdint.h>

// Returns 0 on success, -ENODEV if the encoder timer isn't ready.
int init_dial(void);

// Current encoder count since init. Increases one way, decreases the other.
int32_t dial_get_position(void);

#endif // DIAL_H
