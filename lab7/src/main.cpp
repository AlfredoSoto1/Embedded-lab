#include <Arduino.h>

// ── Activate ONE experiment by uncommenting it ───────────────────────────────
// #define EXPERIMENT_DC_H_BRIDGE_MOTOR
// #define EXPERIMENT_DC_MOTOR_DRIVER
// #define EXPERIMENT_SERVO_MOTOR
#define EXPERIMENT_STEPPER_MOTOR
// ─────────────────────────────────────────────────────────────────────────────

#if defined(EXPERIMENT_DC_H_BRIDGE_MOTOR)
    #include "dc_motor.hpp"
    #define EXP_SETUP  dc_motor_setup
    #define EXP_LOOP   dc_motor_loop

#elif defined(EXPERIMENT_DC_MOTOR_DRIVER)
    #include "dc_motor_driver.hpp"
    #define EXP_SETUP  dc_motor_driver_setup
    #define EXP_LOOP   dc_motor_driver_loop

#elif defined(EXPERIMENT_SERVO_MOTOR)
    #include "servo_motor.hpp"
    #define EXP_SETUP  servo_motor_setup
    #define EXP_LOOP   servo_motor_loop

#elif defined(EXPERIMENT_STEPPER_MOTOR)
    #include "stepper_motor.hpp"
    #define EXP_SETUP  stepper_motor_setup
    #define EXP_LOOP   stepper_motor_loop

#else
    #error "No experiment selected. Uncomment one #define above."
#endif

void setup() { EXP_SETUP(); }
void loop()  { EXP_LOOP();  }

