/**
 * @file io_midi_transport.c
 * @author k-omura
 * @brief MIDI transport implementation
 * @version 0.1
 * @date 2026-07-02
 *
 */

#include "io_midi_transport.h"

#include <string.h>

static uint8_t g_rx_queue[IO_MIDI_RX_QUEUE_CAPACITY][4];
static volatile uint16_t g_rx_wpos;
static volatile uint16_t g_rx_rpos;
static uint8_t g_tx_queue[IO_MIDI_TX_QUEUE_CAPACITY][4];
static uint16_t g_tx_wpos;
static uint16_t g_tx_rpos;
static uint16_t g_tx_count;
static SPI_HandleTypeDef *g_hspi_tx;
static uint8_t g_tx_dma_packet[4];

/**
 * @brief Initializes the MIDI transport module.
 */
void io_midi_transport_init(void)
{
    memset(g_rx_queue, 0, sizeof(g_rx_queue));
    memset(g_tx_queue, 0, sizeof(g_tx_queue));
    g_rx_wpos = 0;
    g_rx_rpos = 0;
    g_tx_wpos = 0;
    g_tx_rpos = 0;
    g_tx_count = 0;
    g_hspi_tx = 0;
    memset(g_tx_dma_packet, 0, sizeof(g_tx_dma_packet));
}

/**
 * @brief Binds the SPI handle for MIDI transmission.
 * @param hspi_tx Pointer to the SPI handle.
 */
void io_midi_transport_bind_tx_spi(SPI_HandleTypeDef *hspi_tx)
{
    g_hspi_tx = hspi_tx;
}

/**
 * @brief Pushes a received MIDI packet into the RX queue.
 * @param packet Pointer to the MIDI packet.
 */
void io_midi_transport_push_rx_packet(const uint8_t packet[4])
{
    uint32_t primask;
    uint16_t next;

    if (packet == 0)
    {
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    next = (uint16_t)((g_rx_wpos + 1U) % IO_MIDI_RX_QUEUE_CAPACITY);
    if (next != g_rx_rpos)
    {
        memcpy(g_rx_queue[g_rx_wpos], packet, 4);
        __DMB();
        g_rx_wpos = next;
    }

    __set_PRIMASK(primask);
}

/**
 * @brief Pops a MIDI packet from the RX queue.
 * @param packet Pointer to the buffer to store the popped MIDI packet.
 * @return 1 if a packet was available and popped, 0 otherwise.
 */
uint8_t io_midi_transport_pop_rx_packet(uint8_t packet[4])
{
    if ((packet == 0) || (g_rx_rpos == g_rx_wpos))
    {
        return 0;
    }

    __DMB();
    memcpy(packet, g_rx_queue[g_rx_rpos], 4);
    g_rx_rpos = (uint16_t)((g_rx_rpos + 1U) % IO_MIDI_RX_QUEUE_CAPACITY);
    return 1;
}

/**
 * @brief Sends a MIDI packet via SPI.
 * @param packet Pointer to the MIDI packet to send.
 */
void io_midi_transport_send_packet(const uint8_t packet[4])
{
    uint16_t next;

    if (packet == 0)
    {
        return;
    }

    next = (uint16_t)((g_tx_wpos + 1U) % IO_MIDI_TX_QUEUE_CAPACITY);
    if (g_tx_count >= IO_MIDI_TX_QUEUE_CAPACITY)
    {
        return;
    }

    memcpy(g_tx_queue[g_tx_wpos], packet, 4);
    g_tx_wpos = next;
    g_tx_count++;
}

/**
 * @brief Services the MIDI transport module. This function should be called periodically to handle MIDI packet transmission.
 */
void io_midi_transport_service(void)
{
    if ((g_hspi_tx == 0) || (g_tx_count == 0U) || (HAL_SPI_GetState(g_hspi_tx) != HAL_SPI_STATE_READY))
    {
        return;
    }

    memcpy(g_tx_dma_packet, g_tx_queue[g_tx_rpos], sizeof(g_tx_dma_packet));
    if (HAL_SPI_Transmit_DMA(g_hspi_tx, g_tx_dma_packet, sizeof(g_tx_dma_packet)) != HAL_OK)
    {
        return;
    }

    g_tx_rpos = (uint16_t)((g_tx_rpos + 1U) % IO_MIDI_TX_QUEUE_CAPACITY);
    g_tx_count--;
}
