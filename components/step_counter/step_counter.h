#ifndef STEP_COUNTER_H
#define STEP_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Start the background step-counting task.
 *        Must be called after acc_init().
 */
esp_err_t step_counter_init(void);

/**
 * @brief Return the total number of steps counted since boot.
 */
uint32_t step_counter_get(void);

/**
 * @brief Reset the step count to zero.
 */
void step_counter_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* STEP_COUNTER_H */
