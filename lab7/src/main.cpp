#include <Arduino.h>

// ── Activate ONE experiment by uncommenting it ───────────────────────────────
#define EXPERIMENT_DC_H_BRIDGE_MOTOR
// ─────────────────────────────────────────────────────────────────────────────

#if defined(EXPERIMENT_DC_H_BRIDGE_MOTOR)
    #include "dc_motor.hpp"
    #define EXP_SETUP  dc_motor_setup
    #define EXP_LOOP   dc_motor_loop

#else
    #error "No experiment selected. Uncomment one #define above."
#endif

void setup() { EXP_SETUP(); }
void loop()  { EXP_LOOP();  }

