#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "sh1107.h"
#include "font8x8_basic.h"

#define tag "SH1107"

static const int GPIO_MOSI = 23;
static const int GPIO_SCLK = 18;
static const int GPIO_CS   = 14;
static const int GPIO_DC   = 27;
static const int GPIO_RESET= 33;

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
//static const int SPI_Frequency = 1000000;
static const int SPI_Frequency = 8000000;

void spi_master_init(SH1107_t * dev)
{
	esp_err_t ret;

	ret = gpio_set_direction( GPIO_CS, GPIO_MODE_OUTPUT );
	ESP_LOGI(tag, "gpio_set_direction=%d",ret);
	assert(ret==ESP_OK);
	gpio_set_level( GPIO_CS, 1 );

	ret = gpio_set_direction( GPIO_DC, GPIO_MODE_OUTPUT );
	ESP_LOGI(tag, "gpio_set_direction=%d",ret);
	assert(ret==ESP_OK);
	gpio_set_level( GPIO_DC, 0 );

   	ret = gpio_set_direction( GPIO_RESET, GPIO_MODE_OUTPUT );
	ESP_LOGI(tag, "gpio_set_direction=%d",ret);
	assert(ret==ESP_OK);
	gpio_set_level( GPIO_RESET, 0 );
	vTaskDelay( pdMS_TO_TICKS( 100 ) );
	gpio_set_level( GPIO_RESET, 1 );

	spi_bus_config_t spi_bus_config = {
		.sclk_io_num = GPIO_SCLK,
		.mosi_io_num = GPIO_MOSI,
		.miso_io_num = -1,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
	};

	ret = spi_bus_initialize( HSPI_HOST, &spi_bus_config, 1 );
	ESP_LOGI(tag, "spi_bus_initialize=%d",ret);
	assert(ret==ESP_OK);

	spi_device_interface_config_t devcfg;
	memset( &devcfg, 0, sizeof( spi_device_interface_config_t ) );
	devcfg.clock_speed_hz = SPI_Frequency;
	devcfg.spics_io_num = GPIO_CS;
	devcfg.queue_size = 1;

	spi_device_handle_t handle;
	ret = spi_bus_add_device( HSPI_HOST, &devcfg, &handle);
	ESP_LOGI(tag, "spi_bus_add_device=%d",ret);
	assert(ret==ESP_OK);
	dev->_SPIHandle = handle;
}


bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength )
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	if ( DataLength > 0 ) {
		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = DataLength * 8;
		SPITransaction.tx_buffer = Data;
		ret = spi_device_transmit( SPIHandle, &SPITransaction );
		//ESP_LOGI(tag, "spi_device_transmit=%d",ret);
		assert(ret==ESP_OK);
	}

	return true;
}

bool spi_master_write_command(SH1107_t * dev, uint8_t Command )
{
	//ESP_LOGI(tag, "spi_master_write_command 0x%x",Command);
	static uint8_t CommandByte = 0;
	CommandByte = Command;
	gpio_set_level( GPIO_DC, SPI_Command_Mode );
	return spi_master_write_byte( dev->_SPIHandle, &CommandByte, 1 );
}

bool spi_master_write_data(SH1107_t * dev, const uint8_t* Data, size_t DataLength )
{
	//ESP_LOGI(tag, "spi_master_write_data 0x%x",Data[0]);
	gpio_set_level( GPIO_DC, SPI_Data_Mode );
	return spi_master_write_byte( dev->_SPIHandle, Data, DataLength );
}

void spi_init(SH1107_t * dev, int width, int height)
{
	dev->_width = width;
	dev->_height = height;
	dev->_pages = height / 8;

	spi_master_write_command(dev, 0xAE);	// Turn display off
	spi_master_write_command(dev, 0xDC);	// Set display start line
	spi_master_write_command(dev, 0x00);	// ...value
	spi_master_write_command(dev, 0x81);	// Set display contrast
	spi_master_write_command(dev, 0x2F);	// ...value
	spi_master_write_command(dev, 0x20);	// Set memory mode
	spi_master_write_command(dev, 0xA0);	// Non-rotated display
	spi_master_write_command(dev, 0xC0);	// Non-flipped vertical
	spi_master_write_command(dev, 0xA8);	// Set multiplex ratio
	spi_master_write_command(dev, 0x7F);	// ...value
	spi_master_write_command(dev, 0xD3);	// Set display offset to zero
	spi_master_write_command(dev, 0x60);	// ...value
	spi_master_write_command(dev, 0xD5);	// Set display clock divider
	spi_master_write_command(dev, 0x51);	// ...value
	spi_master_write_command(dev, 0xD9);	// Set pre-charge
	spi_master_write_command(dev, 0x22);	// ...value
	spi_master_write_command(dev, 0xDB);	// Set com detect
	spi_master_write_command(dev, 0x35);	// ...value
	spi_master_write_command(dev, 0xB0);	// Set page address
	spi_master_write_command(dev, 0xDA);	// Set com pins
	spi_master_write_command(dev, 0x12);	// ...value
	spi_master_write_command(dev, 0xA4);	// output ram to display
	spi_master_write_command(dev, 0xA6);	// Non-inverted display
//	spi_master_write_command(dev, 0xA7);	// Inverted display
	spi_master_write_command(dev, 0xAF);	// Turn display on
}

void display_text(SH1107_t * dev, int page, char * text, int text_len, bool invert)
{
	if (page >= dev->_pages) return;
	int _text_len = text_len;
	if (_text_len > 8) _text_len = 8;

	uint8_t seg = 0;
	uint8_t image[8];
	for (uint8_t i = 0; i < _text_len; i++) {
		memcpy(image, font8x8_basic_tr[(uint8_t)text[i]], 8);
		if (invert) display_invert(image, 8);
		display_image(dev, page, seg, image, 8);
		for(int j=0;j<8;j++) 
			dev->_page[page]._segs[seg+j] = image[j];
		seg = seg + 8;
	}
}

void display_image(SH1107_t * dev, int page, int seg, uint8_t * images, int width)
{
	if (page >= dev->_pages) return;
	if (seg >= dev->_width) return;

	uint8_t columLow = seg & 0x0F;
	uint8_t columHigh = (seg >> 4) & 0x0F;
	//ESP_LOGI(tag, "page=%x columLow=%x columHigh=%x",page,columLow,columHigh);

	// Set Higher Column Start Address for Page Addressing Mode
	spi_master_write_command(dev, (0x10 + columHigh));
	// Set Lower Column Start Address for Page Addressing Mode
	spi_master_write_command(dev, (0x00 + columLow));
	// Set Page Start Address for Page Addressing Mode
	spi_master_write_command(dev, 0xB0 | page);

	spi_master_write_data(dev, images, width);

}

void clear_screen(SH1107_t * dev, bool invert)
{
	char zero[64];
	memset(zero, 0, sizeof(zero));
	for (int page = 0; page < dev->_pages; page++) {
		display_text(dev, page, zero, dev->_width, invert);
	}
}

void clear_line(SH1107_t * dev, int page, bool invert)
{
	char zero[64];
	memset(zero, 0, sizeof(zero));
	display_text(dev, page, zero, dev->_width, invert);
}

void display_page_up(SH1107_t * dev)
{
	for (int page = 1; page < dev->_pages; page++) {
		for(int seg = 0; seg < dev->_width; seg++) {
			dev->_page[page-1]._segs[seg] = dev->_page[page]._segs[seg];
		}
	}
	memset(dev->_page[dev->_pages-1]._segs, 0, dev->_width);
	for (int page = 0; page < dev->_pages; page++) {
		display_image(dev, page, 0, dev->_page[page]._segs, dev->_width);
	}
}


void display_page_down(SH1107_t * dev)
{
	for (int page = dev->_pages-1; page > 0; page--) {
		for(int seg = 0; seg < dev->_width; seg++) {
			dev->_page[page]._segs[seg] = dev->_page[page-1]._segs[seg];
		}
	}
	memset(dev->_page[0]._segs, 0, dev->_width);
	for (int page = 0; page < dev->_pages; page++) {
		display_image(dev, page, 0, dev->_page[page]._segs, dev->_width);
	}
}

void display_contrast(SH1107_t * dev, int contrast) {
	int _contrast = contrast;
	if (contrast < 0x0) _contrast = 0;
	if (contrast > 0xFF) _contrast = 0xFF;

	spi_master_write_command(dev, 0x81);
	spi_master_write_command(dev, _contrast);
}

void display_invert(uint8_t *buf, size_t blen)
{
	uint8_t wk;
	for(int i=0; i<blen; i++){
		wk = buf[i];
		buf[i] = ~wk;
	}
}

void display_fadeout(SH1107_t * dev)
{
	uint8_t image[1];
	for(int page=0; page<dev->_pages; page++) {
		image[0] = 0xFF;
		for(int line=0; line<8; line++) {
			image[0] = image[0] << 1;
			for(int seg=0; seg<dev->_width; seg++) {
				display_image(dev, page, seg, image, 1);
			}
		}
	}
}
