#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#define CONFIG_STACK	0
#define CONFIG_STICKC	0
#define CONFIG_STICK	1

#if CONFIG_STACK
#include "ili9340.h"
#include "fontx.h"
#endif

#if CONFIG_STICKC
#include "axp192.h"
#include "st7735s.h"
#include "fontx.h"
#endif

#if CONFIG_STICK
#include "sh1107.h"
#include "font8x8_basic.h"
#endif

#include "ir_nec.h"

#if CONFIG_STACK
#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	240
#define CS_GPIO		14
#define DC_GPIO		27
#define RESET_GPIO	33
#define BL_GPIO		32
#define FONT_WIDTH	12
#define FONT_HEIGHT	24
#define MAX_CONFIG      20
#define MAX_LINE	8
#define DISPLAY_LENGTH	26
#define GPIO_INPUT_A	GPIO_NUM_39
#define GPIO_INPUT_B	GPIO_NUM_38
#define GPIO_INPUT_C	GPIO_NUM_37
#define RMT_TX_CHANNEL	1			/*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM	GPIO_NUM_26		/*!< GPIO number for transmitter signal */
#endif

#if CONFIG_STICKC
#define SCREEN_WIDTH	80
#define SCREEN_HEIGHT	160
#define FONT_WIDTH	8
#define FONT_HEIGHT	16
#define MAX_CONFIG      20
#define MAX_LINE	8
#define DISPLAY_LENGTH	10
#define GPIO_INPUT_A	GPIO_NUM_37
#define GPIO_INPUT_B	GPIO_NUM_39
#define RMT_TX_CHANNEL	1			/*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM	GPIO_NUM_9		/*!< GPIO number for transmitter signal */
#endif

#if CONFIG_STICK
#define MAX_CONFIG      14
#define MAX_LINE	14
#define DISPLAY_LENGTH	8
#define GPIO_INPUT	GPIO_NUM_35
#define GPIO_BUZZER	GPIO_NUM_26
#define RMT_TX_CHANNEL	1			/*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM	GPIO_NUM_17		/*!< GPIO number for transmitter signal */
#endif


#define CMD_UP		100
#define CMD_DOWN	200
#define CMD_TOP         300
#define CMD_SELECT	400

QueueHandle_t xQueueCmd;

static const char *TAG = "M5Remote";

typedef struct {
    uint16_t command;
    TaskHandle_t taskHandle;
} CMD_t;

typedef struct {
	bool enable;
	char display_text[DISPLAY_LENGTH+1];
	uint16_t ir_cmd;
	uint16_t ir_addr;
} DISPLAY_t;


static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(TAG,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

#if CONFIG_STICK
void buttonStick(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT);
	gpio_set_direction(GPIO_INPUT, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			TickType_t startTick = xTaskGetTickCount();
			while(1) {
				level = gpio_get_level(GPIO_INPUT);
				if (level == 1) break;
				vTaskDelay(1);
			}
			TickType_t endTick = xTaskGetTickCount();
			TickType_t diffTick = endTick-startTick;
			ESP_LOGI(pcTaskGetTaskName(0),"diffTick=%d",diffTick);
			cmdBuf.command = CMD_DOWN;
			if (diffTick > 200) cmdBuf.command = CMD_SELECT;
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}
#endif

#if CONFIG_STICKC || CONFIG_STACK
void buttonA(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = CMD_SELECT;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_A);
	gpio_set_direction(GPIO_INPUT_A, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_A);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT_A);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}
#endif

#if CONFIG_STICKC
void buttonB(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_B);
	gpio_set_direction(GPIO_INPUT_B, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_B);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			cmdBuf.command = CMD_DOWN;
			TickType_t startTick = xTaskGetTickCount();
			while(1) {
				level = gpio_get_level(GPIO_INPUT_B);
				if (level == 1) break;
				vTaskDelay(1);
			}
			TickType_t endTick = xTaskGetTickCount();
			TickType_t diffTick = endTick-startTick;
			ESP_LOGI(pcTaskGetTaskName(0),"diffTick=%d",diffTick);
			cmdBuf.command = CMD_DOWN;
			if (diffTick > 200) cmdBuf.command = CMD_TOP;
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}
#endif

#if CONFIG_STACK
void buttonB(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = CMD_DOWN;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_B);
	gpio_set_direction(GPIO_INPUT_B, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_B);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT_B);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}
#endif

#if CONFIG_STACK
void buttonC(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = CMD_UP;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_C);
	gpio_set_direction(GPIO_INPUT_C, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_C);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT_C);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}
#endif

static int parseLine(char *line, int size1, int size2, char arr[size1][size2])
{
	ESP_LOGD(TAG, "line=[%s]", line);
	int dst = 0;
	int pos = 0;
	int llen = strlen(line);
	bool inq = false;

	for(int src=0;src<llen;src++) {
		char c = line[src];
		ESP_LOGD(TAG, "src=%d c=%c", src, c);
		if (c == ',') {
			if (inq) {
				if (pos == (size2-1)) continue;
				arr[dst][pos++] = line[src];
				arr[dst][pos] = 0;
			} else {
				ESP_LOGD(TAG, "arr[%d]=[%s]",dst,arr[dst]);
				dst++;
				if (dst == size1) break;
				pos = 0;
			}

		} else if (c == ';') {
			if (inq) {
				if (pos == (size2-1)) continue;
				arr[dst][pos++] = line[src];
				arr[dst][pos] = 0;
			} else {
				ESP_LOGD(TAG, "arr[%d]=[%s]",dst,arr[dst]);
				dst++;
				break;
			}

		} else if (c == '"') {
			inq = !inq;

		} else if (c == '\'') {
			inq = !inq;

		} else {
			if (pos == (size2-1)) continue;
			arr[dst][pos++] = line[src];
			arr[dst][pos] = 0;
		}
	}

	return dst;
}


static int readDefineFile(DISPLAY_t *display, size_t maxLine, size_t maxText) {
	int readLine = 0;
	ESP_LOGI(pcTaskGetTaskName(0), "Reading file:maxText=%d",maxText);
	FILE* f = fopen("/spiffs/Display.def", "r");
	if (f == NULL) {
	    ESP_LOGE(pcTaskGetTaskName(0), "Failed to open define file for reading");
	    ESP_LOGE(pcTaskGetTaskName(0), "Please make Display.def");
	    return 0;
	}
	char line[64];
	char result[10][32];
	while (1){
		if ( fgets(line, sizeof(line) ,f) == 0 ) break;
		// strip newline
		char* pos = strchr(line, '\n');
		if (pos) {
			*pos = '\0';
		}
		ESP_LOGI(pcTaskGetTaskName(0), "line=[%s]", line);
		if (strlen(line) == 0) continue;
		if (line[0] == '#') continue;

#if 0
		// %s is divided automatically by a space character in scanf.
		// convert comma to space
		for(int index=0; index<strlen(line); index++) {
			if (line[index] == ',') line[index] = ' ';
		}
		// change from maxText character to space.
		for(int index=0; index<strlen(line); index++) {
			if (line[index] == ' ') break;
			if (index >= maxText) line[index] = ' ';
		}
		ESP_LOGI(pcTaskGetTaskName(0), "edited line=[%s]", line);
		uint32_t cmd;
		uint32_t addr;
		sscanf(line, "%s %x %x",display[readLine].display_text, &cmd, &addr);
		display[readLine].ir_cmd = cmd;
		display[readLine].ir_addr = addr;
#endif

		int ret = parseLine(line, 10, 32, result);
		ESP_LOGI(TAG, "parseLine=%d", ret);
		for(int i=0;i<ret;i++) ESP_LOGI(TAG, "result[%d]=[%s]", i, &result[i][0]);
		display[readLine].enable = true;
		strlcpy(display[readLine].display_text, &result[0][0], maxText);
		display[readLine].ir_cmd = strtol(&result[1][0], NULL, 16);
		display[readLine].ir_addr = strtol(&result[2][0], NULL, 16);

		readLine++;
		if (readLine == maxLine) break;
	}
	fclose(f);
	return readLine;
}

#if CONFIG_STICKC || CONFIG_STACK
void tft(void *pvParameters)
{
	// set font file
#if CONFIG_STICKC
	FontxFile fxG[2];
	InitFontx(fxG,"/spiffs/ILGH16XB.FNT",""); // 8x16Dot Gothic
	FontxFile fxM[2];
	InitFontx(fxM,"/spiffs/ILMH16XB.FNT",""); // 8x16Dot Mincyo
#endif

#if CONFIG_STACK
	FontxFile fxG[2];
	InitFontx(fxG,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
	FontxFile fxM[2];
	InitFontx(fxM,"/spiffs/ILMH24XB.FNT",""); // 12x24Dot Mincyo
#endif

	// Setup IR transmitter
	nec_tx_init(RMT_TX_CHANNEL, RMT_TX_GPIO_NUM);
	int channel = RMT_TX_CHANNEL;
	size_t size = sizeof(rmt_item32_t) * NEC_DATA_ITEM_NUM;
	//each item represent a cycle of waveform.
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	//int item_num = NEC_DATA_ITEM_NUM;
	memset((void*) item, 0, size);
	ESP_LOGI(pcTaskGetTaskName(0), "Setup IR transmitter done");

	// Setup Screen
#if CONFIG_STICKC
	ST7735_t dev;
	spi_master_init(&dev);
	lcdInit(&dev, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif

#if CONFIG_STACK
	TFT_t dev;
	spi_master_init(&dev, CS_GPIO, DC_GPIO, RESET_GPIO, BL_GPIO);
	lcdInit(&dev, 0x9341, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
#endif
	ESP_LOGI(pcTaskGetTaskName(0), "Setup Screen done");

	// Read display information
	DISPLAY_t display[MAX_CONFIG];
	for(int i=0;i<MAX_CONFIG;i++) display[i].enable = false;
	int readLine = readDefineFile(display, MAX_CONFIG, DISPLAY_LENGTH);
	ESP_LOGI(pcTaskGetTaskName(0), "readLine=%d",readLine);
	if (readLine == 0) {
		while(1) { vTaskDelay(1); }
	}
	for(int i=0;i<readLine;i++) {
		ESP_LOGI(pcTaskGetTaskName(0), "display[%d].display_text=[%s]",i, display[i].display_text);
		ESP_LOGI(pcTaskGetTaskName(0), "display[%d].ir_cmd=[%x]",i, display[i].ir_cmd);
		ESP_LOGI(pcTaskGetTaskName(0), "display[%d].ir_addr=[%x]",i, display[i].ir_addr);
	}

	// Initial Screen
	uint16_t color;
	uint8_t ascii[DISPLAY_LENGTH+1];
	uint16_t ypos;
	lcdFillScreen(&dev, BLACK);
	color = RED;
	lcdSetFontDirection(&dev, 0);
	ypos = FONT_HEIGHT-1;
#if CONFIG_STICKC
	strcpy((char *)ascii, "M5 StickC");
#endif
#if CONFIG_STACK
	strcpy((char *)ascii, "M5 Stack");
#endif
	lcdDrawString(&dev, fxG, 0, ypos, ascii, color);

	int offset = 0;
	for(int i=0;i<MAX_LINE;i++) {
		ypos = FONT_HEIGHT * (i+3) - 1;
		ascii[0] = 0;
		if (display[i+offset].enable) strcpy((char *)ascii, display[i+offset].display_text);
		if (i == 0) {
			lcdDrawString(&dev, fxM, 0, ypos, ascii, YELLOW);
		} else {
			lcdDrawString(&dev, fxG, 0, ypos, ascii, CYAN);
		}
	} // end while

	int selected = 0;
	CMD_t cmdBuf;
	while(1) {
		xQueueReceive(xQueueCmd, &cmdBuf, portMAX_DELAY);
		ESP_LOGI(pcTaskGetTaskName(0),"cmdBuf.command=%d", cmdBuf.command);
		if (cmdBuf.command == CMD_DOWN) {
			ESP_LOGI(pcTaskGetTaskName(0), "selected=%d offset=%d readLine=%d",selected, offset, readLine);
			if ((selected+offset+1) == readLine) continue;

			ypos = FONT_HEIGHT * (selected+3) - 1;
			strcpy((char *)ascii, display[selected+offset].display_text);
			lcdDrawString(&dev, fxM, 0, ypos, ascii, BLACK);
			lcdDrawString(&dev, fxG, 0, ypos, ascii, CYAN);

			if (selected+1 == MAX_LINE) {
				lcdDrawFillRect(&dev, 0, FONT_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BLACK);
				offset++;
				for(int i=0;i<MAX_LINE;i++) {
					ypos = FONT_HEIGHT * (i+3) - 1;
					ascii[0] = 0;
					if (display[i+offset].enable) strcpy((char *)ascii, display[i+offset].display_text);
					lcdDrawString(&dev, fxG, 0, ypos, ascii, CYAN);
				} // end while
			} else {
				selected++;
			}
			ypos = FONT_HEIGHT * (selected+3) - 1;
			strcpy((char *)ascii, display[selected+offset].display_text);
			lcdDrawString(&dev, fxG, 0, ypos, ascii, BLACK);
			lcdDrawString(&dev, fxM, 0, ypos, ascii, YELLOW);

		} else if (cmdBuf.command == CMD_UP) {
			ESP_LOGI(pcTaskGetTaskName(0), "selected=%d offset=%d",selected, offset);
			if (selected+offset == 0) continue;

			ypos = FONT_HEIGHT * (selected+3) - 1;
			strcpy((char *)ascii, display[selected+offset].display_text);
			lcdDrawString(&dev, fxM, 0, ypos, ascii, BLACK);
			lcdDrawString(&dev, fxG, 0, ypos, ascii, CYAN);

			if (offset > 0) {
				lcdDrawFillRect(&dev, 0, FONT_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BLACK);
				offset--;
				for(int i=0;i<MAX_LINE;i++) {
					ypos = FONT_HEIGHT * (i+3) - 1;
					ascii[0] = 0;
					if (display[i+offset].enable) strcpy((char *)ascii, display[i+offset].display_text);
					lcdDrawString(&dev, fxG, 0, ypos, ascii, CYAN);
				} // end while
			} else {
				selected--;
			}
			ypos = FONT_HEIGHT * (selected+3) - 1;
			strcpy((char *)ascii, display[selected+offset].display_text);
			lcdDrawString(&dev, fxG, 0, ypos, ascii, BLACK);
			lcdDrawString(&dev, fxM, 0, ypos, ascii, YELLOW);

		} else if (cmdBuf.command == CMD_TOP) {
			offset = 0;
			selected = 0;
			lcdDrawFillRect(&dev, 0, FONT_HEIGHT-1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BLACK);
			for(int i=0;i<MAX_LINE;i++) {
				ypos = FONT_HEIGHT * (i+3) - 1;
				ascii[0] = 0;
				if (display[i+offset].enable) strcpy((char *)ascii, display[i+offset].display_text);
				if (i == 0) {
					lcdDrawString(&dev, fxM, 0, ypos, ascii, YELLOW);
				} else {
					lcdDrawString(&dev, fxG, 0, ypos, ascii, CYAN);
				}
			} // end while

		} else if (cmdBuf.command == CMD_SELECT) {
			ESP_LOGI(pcTaskGetTaskName(0), "selected=%d offset=%d",selected, offset);
			//To build a series of waveforms.
			ESP_LOGI(pcTaskGetTaskName(0), "ir_cmd=0x%x",display[selected+offset].ir_cmd);
			ESP_LOGI(pcTaskGetTaskName(0), "ir_addr=0x%x",display[selected+offset].ir_addr);
			uint16_t cmd = display[selected+offset].ir_cmd;
			uint16_t addr = display[selected+offset].ir_addr;;
			int ret = nec_build_items(channel, item, ((~addr) << 8) | addr, ((~cmd) << 8) |  cmd);
			ESP_LOGI(pcTaskGetTaskName(0), "nec_build_items ret=%d",ret);
			if (ret != NEC_DATA_ITEM_NUM) {
				ESP_LOGE(pcTaskGetTaskName(0), "nec_build_items fail");
			} else {
				//To send data according to the waveform items.
				rmt_write_items(channel, item, NEC_DATA_ITEM_NUM, true);
				//Wait until sending is done.
				rmt_wait_tx_done(channel, portMAX_DELAY);
			}

		}
	}

	// nerver reach
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}
#endif


#if CONFIG_STICK
void buzzerON(void) {
	gpio_pad_select_gpio( GPIO_BUZZER );
	gpio_set_direction( GPIO_BUZZER, GPIO_MODE_OUTPUT );
	gpio_set_level( GPIO_BUZZER, 0 );
	for(int i=0;i<50;i++){
		gpio_set_level( GPIO_BUZZER, 1 );
		vTaskDelay(1);
		gpio_set_level( GPIO_BUZZER, 0 );
		vTaskDelay(1);
	}
}


void tft(void *pvParameters)
{
	// Setup IR transmitter
	nec_tx_init(RMT_TX_CHANNEL, RMT_TX_GPIO_NUM);
	int channel = RMT_TX_CHANNEL;
	size_t size = sizeof(rmt_item32_t) * NEC_DATA_ITEM_NUM;
	//each item represent a cycle of waveform.
	rmt_item32_t* item = (rmt_item32_t*) malloc(size);
	//int item_num = NEC_DATA_ITEM_NUM;
	memset((void*) item, 0, size);
	ESP_LOGI(pcTaskGetTaskName(0), "Setup IR transmitter done");

	// Setup Screen
	SH1107_t dev;
	spi_master_init(&dev);
	spi_init(&dev, 64, 128);
	ESP_LOGI(pcTaskGetTaskName(0), "Setup Screen done");

	// Read display information
	DISPLAY_t display[MAX_CONFIG];
	for(int i=0;i<MAX_CONFIG;i++) display[i].enable = false;
	int readLine = readDefineFile(display, MAX_CONFIG, DISPLAY_LENGTH);
	if (readLine == 0) {
		while(1) { vTaskDelay(1); }
	}
	for(int i=0;i<readLine;i++) {
		ESP_LOGI(pcTaskGetTaskName(0), "display[%d].display_text=[%s]",i, display[i].display_text);
		ESP_LOGI(pcTaskGetTaskName(0), "display[%d].ir_cmd=[%d]",i, display[i].ir_cmd);
		ESP_LOGI(pcTaskGetTaskName(0), "display[%d].ir_addr=[%d]",i, display[i].ir_addr);
	}

	// Initial Screen
	clear_screen(&dev, false);
	display_contrast(&dev, 0xff);
	char ascii[DISPLAY_LENGTH+1];
        uint16_t ypos;
	strcpy(ascii, "M5 Stick");
	display_text(&dev, 0, ascii, 8, false);

	for(int i=0;i<MAX_LINE;i++) {
		ypos = i + 2;
		ascii[0] = 0;
		if (display[i].enable) strcpy(ascii, display[i].display_text);
		if (i == 0) {
			display_text(&dev, ypos, ascii, strlen(ascii), true);
		} else {
			display_text(&dev, ypos, ascii, strlen(ascii), false);
		}
	} // end for

	int selected = 0;
	CMD_t cmdBuf;
	while(1) {
		xQueueReceive(xQueueCmd, &cmdBuf, portMAX_DELAY);
		ESP_LOGI(pcTaskGetTaskName(0),"cmdBuf.command=%d", cmdBuf.command);
		if (cmdBuf.command == CMD_DOWN) {
			strcpy(ascii, display[selected].display_text);
			ypos = selected + 2;
			display_text(&dev, ypos, ascii, strlen(ascii), false);

			selected++;
			if (selected == readLine) selected = 0;
			strcpy(ascii, display[selected].display_text);
			ypos = selected + 2;
			display_text(&dev, ypos, ascii, strlen(ascii), true);

		} else if (cmdBuf.command == CMD_SELECT) {
			//To build a series of waveforms.
			uint16_t cmd = display[selected].ir_cmd;
			uint16_t addr = display[selected].ir_addr;;
			int i = nec_build_items(channel, item, ((~addr) << 8) | addr, ((~cmd) << 8) |  cmd);
			if (i != NEC_DATA_ITEM_NUM) {
				ESP_LOGW(pcTaskGetTaskName(0), "nec_build_items fail");
			} else {
				//To send data according to the waveform items.
				rmt_write_items(channel, item, NEC_DATA_ITEM_NUM, true);
				//Wait until sending is done.
				rmt_wait_tx_done(channel, portMAX_DELAY);
				buzzerON();
			}

		}
	}

	// nerver reach
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}
#endif

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 6,
		.format_if_mount_failed =true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret =esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret ==ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret== ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

	SPIFFS_Directory("/spiffs");

	/* Create Queue */
	xQueueCmd = xQueueCreate( 10, sizeof(CMD_t) );
	configASSERT( xQueueCmd );

#if CONFIG_STICKC
	// power on
	i2c_master_init();
	AXP192_PowerOn();
#endif

	xTaskCreate(tft, "TFT", 1024*4, NULL, 2, NULL);

#if CONFIG_STACK
	xTaskCreate(buttonA, "SELECT", 1024*4, NULL, 2, NULL);
	xTaskCreate(buttonB, "DOWN", 1024*4, NULL, 2, NULL);
	xTaskCreate(buttonC, "UP", 1024*4, NULL, 2, NULL);
#endif

#if CONFIG_STICKC
	xTaskCreate(buttonA, "SELECT", 1024*4, NULL, 2, NULL);
	xTaskCreate(buttonB, "DOWN", 1024*4, NULL, 2, NULL);
#endif

#if CONFIG_STICK
	xTaskCreate(buttonStick, "BUTTON", 1024*4, NULL, 2, NULL);
#endif
}

