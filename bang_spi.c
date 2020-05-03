#include <bcm2835.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "bang_spi.h"

static uint8_t _sck, _cs, _mosi, _miso;
static uint8_t _cpol = 0, _cpha = 0;
static uint16_t _clk_divider = 65535;

static BANG_SPI_BIT_ORDER _bit_order = BANG_SPI_BIT_ORDER_MSBFIRST;

/*
set and initialize pins to use for SPIing
*/
uint8_t bang_spi_init(uint8_t sck, uint8_t cs, uint8_t mosi, uint8_t miso)
{
    _sck = sck;
    _cs = cs;
    _mosi = mosi;
    _miso = miso;

    if (!bcm2835_init())
        return 1;

    bcm2835_gpio_fsel(sck, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(_sck, _cpol ? HIGH : LOW );

    bcm2835_gpio_fsel(_cs, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set(_cs);


    bcm2835_gpio_fsel(_mosi, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_clr(_mosi);

    bcm2835_gpio_fsel(_miso, BCM2835_GPIO_FSEL_INPT);
}

void bang_spi_exit()
{
    bcm2835_gpio_fsel(_sck, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(_cs, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(_mosi, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(_miso, BCM2835_GPIO_FSEL_INPT);

    bcm2835_close();
}

uint8_t bang_spi_transfer(uint8_t out)
{
    uint8_t in = 0;
    bang_spi_transfer_n(&out, &in, 1);
    return in;
}

void bang_spi_reverse_bits(uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t v = 0;
        uint8_t *current = buf + i;
        v |= (*current & 0x80) >> 7;
        v |= (*current & 0x40) >> 5;
        v |= (*current & 0x20) >> 3;
        v |= (*current & 0x10) >> 1;
        v |= (*current & 0x08) << 1;
        v |= (*current & 0x04) << 3;
        v |= (*current & 0x02) << 5;
        v |= (*current & 0x01) << 7;
        *current = v;
    }
}

/*
transfer more than one byte
use this for multi byte operations
example: 
1byte cmd from Master to Slave that prompts a 2 byte response
MOSI [0xcd][0x00][0x00] inbuf
MISO [0xXX][0xd1][0xd2] outbuf (after transfer_n returns)
=> bang_spi_transfer_n(&outbuf, &inbuf, 3)

CS stays LOW for the duration of the transfer
*/
uint8_t* bang_spi_transfer_n(uint8_t *outbuf, uint8_t *inbuf, uint8_t len)
{
    // empty inbuf
    memset(inbuf, 0x00, len);

    // convert outbuf to LSBFIRST
    if (_bit_order == BANG_SPI_BIT_ORDER_LSBFIRST)
    {
        bang_spi_reverse_bits(outbuf, len);
    }

    // get clock's current idle state
    int clock = _cpol ? HIGH : LOW;

    // pull down CS
    bcm2835_gpio_write(_cs, LOW);

    for (int8_t k = 0; k < len; k++)
    {
        for (int8_t i = 7; i >= 0; i--)
        {
            if (_cpha)
            {
                // set clock to active
                clock ^= 1;        
                bcm2835_gpio_write(_sck, clock);
            }

            // tx 1bit
            bcm2835_gpio_write(_mosi, (outbuf[k] >> i & 1) ? HIGH : LOW);

            bang_spi_wait();

            // flip the clock signal
            clock ^= 1;        
            bcm2835_gpio_write(_sck, clock);

            bang_spi_wait();

            // rx 1bit
            inbuf[k] |= (bcm2835_gpio_lev(_miso) == HIGH ? 1 : 0) << i;
            
            if (!_cpha)
            {
                // set clock to idle
                clock ^= 1;
                bcm2835_gpio_write(_sck, clock);
            }
        }
    }

    // pull up CS
    bcm2835_gpio_write(_cs, HIGH);

    /* is inbuf conversion desirable?
    // convert value to LSBFIRST
    if (_bit_order == LSBFIRST)
    {
        bang_spi_reverse_bits(inbuf, len);
    }
    */

    return inbuf;
}

// TODO disassemble and check how many instructions the loop consists of
// how many CPU cycles does the loop take? = number_of_instructions?
// can NOP be optimized away by the CPU? (NOT the compiler)
void bang_spi_wait()
{
    uint_fast16_t i = 0;
    for (i; i < _clk_divider; i++)
    {
        __asm__("nop");
    }
}

/*
default: MSBFIRST
*/
void bang_spi_bit_order(uint8_t order)
{
    _bit_order = order;
}

/*
Mode | CPOL CPHA
----------------
   0 |    0    0
   1 |    0    1
   2 |    1    0
   3 |    1    1
*/
void bang_spi_mode(uint8_t mode)
{
    if (mode >= 0 && mode <= 3)
    {
        _cpol = mode & 0x02;
        _cpha = mode & 0x01;
    } else
    {
        _cpol = _cpha = 0;
    }

    // set clock to idle 
    bcm2835_gpio_write(_sck, _cpol ? HIGH : LOW );
}

void bang_spi_clock_divider(uint16_t divider)
{
    // divide by 2 to compensate for the fact
    // that our busy loop takes at least two instructions
    // so this might get our resulting SPI clock freq closer to
    // the theoretical cpu_freq / divider
    _clk_divider = divider >> 1;
}
