#include <Arduino.h>

// ── Activate ONE experiment by uncommenting it ───────────────────────────────
// #define EXPERIMENT_SCROLL_POLLING
// #define EXPERIMENT_SCROLL_INTERRUPT
// #define EXPERIMENT_PWM
// #define EXPERIMENT_RGB_PWM
#define EXPERIMENT_DIMMER
// ─────────────────────────────────────────────────────────────────────────────

#if defined(EXPERIMENT_SCROLL_POLLING)
    #include "scroll_polling.h"
    #define EXP_SETUP  scroll_polling_setup
    #define EXP_LOOP   scroll_polling_loop

#elif defined(EXPERIMENT_SCROLL_INTERRUPT)
    #include "scroll_interrupt.h"
    #define EXP_SETUP  scroll_interrupt_setup
    #define EXP_LOOP   scroll_interrupt_loop

#elif defined(EXPERIMENT_PWM)
    #include "pwm_experiment.h"
    #define EXP_SETUP  pwm_setup
    #define EXP_LOOP   pwm_loop

#elif defined(EXPERIMENT_RGB_PWM)
    #include "rgb_pwm.h"
    #define EXP_SETUP  rgb_pwm_setup
    #define EXP_LOOP   rgb_pwm_loop

#elif defined(EXPERIMENT_DIMMER)
    #include "dimmer.h"
    #define EXP_SETUP  dimmer_setup
    #define EXP_LOOP   dimmer_loop

#else
    #error "No experiment selected. Uncomment one #define above."
#endif

void setup() { EXP_SETUP(); }
void loop()  { EXP_LOOP();  }

