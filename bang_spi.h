#ifndef BANG_SPI_H 
#define BANG_SPI_H

/*
Software SPI Implementation (BitBanging)

tuned to RaspberryPI3 clock of 1.2GHz

TODO: measure actual CPU freqencies, might go from 600-1200MHz
      and make sure we set the divider accordingly

wait() loop can't have less than two instructions, 
therefor we calculate a reasonable clock divider
for 10MHz SPI CLK like 1200/10 = 120 

TODO:
fun exercise: measure the actual frequency, maybe by setting a large divider
              and have the signal activate an Interrupt on another system!)
*/

typedef enum 
{
    BANG_SPI_BIT_ORDER_MSBFIRST,
    BANG_SPI_BIT_ORDER_LSBFIRST,
} BANG_SPI_BIT_ORDER;

uint8_t bang_spi_init(uint8_t sck, uint8_t cs, uint8_t mosi, uint8_t miso);
void bang_spi_exit();
uint8_t bang_spi_transfer(uint8_t value);
void bang_spi_reverse_bits(uint8_t *buf, uint8_t len);
uint8_t* bang_spi_transfer_n(uint8_t *outbuf, uint8_t *inbuf, uint8_t len);
void bang_spi_wait();
void bang_spi_bit_order(uint8_t order);
void bang_spi_mode(uint8_t mode);
void bang_spi_clock_divider(uint16_t divider);

#endif /* BANG_SPI_H */
