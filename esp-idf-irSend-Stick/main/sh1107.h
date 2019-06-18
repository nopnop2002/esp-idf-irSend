#ifndef MAIN_SH1107_H_
#define MAIN_SH1107_H_

#include "driver/spi_master.h"

typedef struct {
	uint8_t _segs[64];
} PAGE_t;

typedef struct {
	int _width;
	int _height;
	int _pages;
	spi_device_handle_t _SPIHandle;
	PAGE_t _page[16];
} SH1107_t;

void spi_master_init(SH1107_t * dev);
bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength );
bool spi_master_write_command(SH1107_t * dev, uint8_t Command );
bool spi_master_write_data(SH1107_t * dev, const uint8_t* Data, size_t DataLength );
void spi_init(SH1107_t * dev, int width, int height);
void display_text(SH1107_t * dev, int page, char * text, int text_len, bool invert);
void display_image(SH1107_t * dev, int page, int seg, uint8_t * images, int width);
void clear_screen(SH1107_t * dev, bool invert);
void clear_line(SH1107_t * dev, int page, bool invert);
void display_contrast(SH1107_t * dev, int contrast);
void display_page_up(SH1107_t * dev);
void display_page_down(SH1107_t * dev);
void display_invert(uint8_t *buf, size_t blen);
void display_fadeout(SH1107_t * dev);
#endif /* MAIN_SH1107_H_ */

