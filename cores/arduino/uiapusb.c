#include <ch32fun.h>
#include <stdio.h>
#include <stdint.h>
#include "rv003usb.h"
#include "usb_config.h"
#include "uiapusb.h"

#define UIAPUSB_RX_BUF_SIZE 256

static volatile uint8_t rxbuf[UIAPUSB_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

static inline uint16_t rb_next(uint16_t v)
{
    return (uint16_t)((v + 1) % UIAPUSB_RX_BUF_SIZE);
}

static inline int rb_count(void)
{
    if (rx_head >= rx_tail) return (int)(rx_head - rx_tail);
    return (int)(UIAPUSB_RX_BUF_SIZE - rx_tail + rx_head);
}

/*
 * demo_terminal.c と同じ受信入口
 * WebLink tタブから来たデータはここに入る
 */
void handle_debug_input(int numbytes, uint8_t *data)
{
    for (int i = 0; i < numbytes; i++)
    {
        uint16_t n = rb_next(rx_head);
        if (n == rx_tail)
        {
            break; // overflowしたら捨てる
        }
        rxbuf[rx_head] = data[i];
        rx_head = n;
    }
}

static uint8_t g_uiap_core_inited = 0;
static uint8_t g_uiap_usb_inited = 0;

/*
 * D- (USB_PIN_DM) を出力 Low に駆動して、
 * ハードワイヤのプルアップをオーバーライドする。
 * ホストはデバイス未接続と認識する。
 */
static void usb_disconnect(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
    /* PD4 (USB_PIN_DM=4): 出力 push-pull, 2MHz */
    GPIOD->CFGLR = (GPIOD->CFGLR & ~(0xf << (USB_PIN_DM * 4)))
                  | ((GPIO_Speed_2MHz | GPIO_CNF_OUT_PP) << (USB_PIN_DM * 4));
    GPIOD->BCR = 1 << USB_PIN_DM;   /* D- = Low */
}

void uiap_core_begin(void)
{
    if (g_uiap_core_inited) return;
    SystemInit();
    usb_disconnect();
    SysTick->CNT = 0;
    g_uiap_core_inited = 1;
}

void uiapusb_begin(void)
{
    if (g_uiap_usb_inited) return;
    uiap_core_begin();
    Delay_Ms(1);
    usb_setup();
    g_uiap_usb_inited = 1;
}

int uiapusb_available(void)
{
    poll_input();
    return rb_count();
}

int uiapusb_read(uint8_t *buf, int maxlen)
{
    poll_input();
    int n = 0;
    while (n < maxlen && rx_tail != rx_head)
    {
        buf[n++] = rxbuf[rx_tail];
        rx_tail = rb_next(rx_tail);
    }
    return n;
}

int uiapusb_write(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        putchar(buf[i]);
    }
    return len;
}