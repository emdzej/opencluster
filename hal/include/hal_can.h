/**
 * @file hal_can.h
 * OpenCluster CAN bus HAL interface.
 *
 * Abstracts CAN send/receive so the same protocol code works over
 * UDP multicast (desktop) or TWAI (ESP32).
 */

#ifndef HAL_CAN_H
#define HAL_CAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A single CAN 2.0B frame. */
typedef struct {
    uint32_t id;        /**< CAN ID (11-bit standard, bits 0-10) */
    uint8_t  len;       /**< Data length code (0-8) */
    uint8_t  data[8];   /**< Payload bytes */
} can_frame_t;

/**
 * Initialize the CAN transport.
 * @return 0 on success, negative on error.
 */
int hal_can_init(void);

/**
 * Send a CAN frame.
 * @param frame  Pointer to the frame to send.
 * @return 0 on success, negative on error.
 */
int hal_can_send(const can_frame_t *frame);

/**
 * Receive a CAN frame (blocking with timeout).
 * @param frame       Output: received frame.
 * @param timeout_ms  Maximum time to wait in milliseconds. 0 = non-blocking.
 * @return 0 on success, -1 on timeout, negative on error.
 */
int hal_can_receive(can_frame_t *frame, uint32_t timeout_ms);

/**
 * Shut down the CAN transport.
 */
void hal_can_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_CAN_H */
