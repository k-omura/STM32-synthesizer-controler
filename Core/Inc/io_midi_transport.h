/**
 * @file io_midi_transport.h
 * @author k-omura
 * @brief MIDI transport definitions and utilities
 * @version 0.1
 * @date 2026-07-02
 *
 */

#ifndef IO_MIDI_TRANSPORT_H
#define IO_MIDI_TRANSPORT_H

#include <stdint.h>
#include "main.h"

#define IO_MIDI_RX_QUEUE_CAPACITY (512)
#define IO_MIDI_TX_QUEUE_CAPACITY (512)

void io_midi_transport_init(void);
void io_midi_transport_bind_tx_spi(SPI_HandleTypeDef *hspi_tx);
void io_midi_transport_push_rx_packet(const uint8_t packet[4]);
uint8_t io_midi_transport_pop_rx_packet(uint8_t packet[4]);
void io_midi_transport_send_packet(const uint8_t packet[4]);
void io_midi_transport_service(void);

#endif
