#include "gdb-packet.h"
#include "nuclei_sdk_hal.h"
#include "pt.h"

void rvl_transport_init(void)
{
    /* enable USART clock */
    rcu_periph_clock_enable(RCU_UART4);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_12);
    gpio_init(GPIOD, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, GPIO_PIN_2);
    /* USART configure */
    usart_deinit(UART4);
    usart_baudrate_set(UART4, 115200U);
    usart_word_length_set(UART4, USART_WL_8BIT);
    usart_stop_bit_set(UART4, USART_STB_1BIT);
    usart_parity_config(UART4, USART_PM_NONE);
    usart_hardware_flow_rts_config(UART4, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(UART4, USART_CTS_DISABLE);
    usart_receive_config(UART4, USART_RECEIVE_ENABLE);
    usart_transmit_config(UART4, USART_TRANSMIT_ENABLE);
    usart_enable(UART4);
}

PT_THREAD(rvl_transport_poll(void))
{
    const uint8_t *send_buffer;
    size_t send_len;
    uint8_t ch;
    if (SET == usart_flag_get(UART4, USART_FLAG_RBNE)) {
        ch = usart_data_receive(UART4);
        gdb_packet_process_command(&ch, 1);
    }

    send_buffer = gdb_packet_get_response(&send_len);
    if (NULL != send_buffer) {
        for (int i = 0;i < send_len;i++) {
            usart_write(UART4, send_buffer[i]);
        }
        gdb_packet_release_response();
    }
}