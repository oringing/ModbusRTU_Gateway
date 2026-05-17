// Bsp/Inc/key.h
#ifndef __KEY_H
#define __KEY_H

#include "stm32f1xx_hal.h"

/* ================= Key Hardware Configuration ================= */
/* Define Key GPIO ports and pins here when hardware is connected */
// #define KEY_GPIO_PORT    GPIOA
// #define KEY_PIN          GPIO_PIN_0

/* ================= Key Logic Constants ================= */
/* Debounce time in milliseconds (Typical: 20-50ms) */
#define KEY_DEBOUNCE_TIME_MS (20U)

/* Long press detection threshold in milliseconds (Typical: 2000ms) */
#define KEY_LONG_PRESS_TIME_MS (2000U)

/* Key state definitions */
typedef enum { KEY_STATE_IDLE = 0, KEY_STATE_PRESSED, KEY_STATE_LONG_PRESSED } KeyState_t;

/* Function prototypes */
void    BSP_Key_Init(void);
uint8_t BSP_Key_Scan(void); /* Returns 0 if no key pressed, or key ID */

#endif /* __KEY_H */