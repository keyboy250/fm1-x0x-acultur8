/* Standalone production-USB fixtures do not always link the sequencer. */
#include "../../firmware/src/timing_rate.h"
volatile timing_rate_state_t timing_rate_state __attribute__((weak))={X0X_TIMING_RATE_HZ,0,0,0,1,24000000.0f/X0X_TIMING_RATE_HZ};
