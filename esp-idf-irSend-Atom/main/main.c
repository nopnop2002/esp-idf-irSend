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
#include "driver/rmt.h"
#include "ir_tools.h"


#define GPIO_INPUT GPIO_NUM_39
#define RMT_TX_CHANNEL 1 /*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM	GPIO_NUM_12 /*!< GPIO number for transmitter signal */
#define MAX_CONFIG 20
#define MAX_CHARACTER 16


#define CMD_UP 100
#define CMD_DOWN 200
#define CMD_TOP 300
#define CMD_BOTTOM 400
#define CMD_SELECT 500

QueueHandle_t xQueueCmd;

static const char *TAG = "M5Remote";

typedef struct {
	uint16_t command;
	TaskHandle_t taskHandle;
} CMD_t;

typedef struct {
	bool enable;
	char display_text[MAX_CHARACTER+1];
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

void buttonStick(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();
	cmdBuf.command = CMD_SELECT;

	// set the GPIO as a input
	gpio_reset_pin(GPIO_INPUT);
	gpio_set_direction(GPIO_INPUT, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT);
		if (level == 0) {
			ESP_LOGI(pcTaskGetName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}


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
	ESP_LOGI(pcTaskGetName(0), "Reading file:maxText=%d",maxText);
	FILE* f = fopen("/spiffs/Display.def", "r");
	if (f == NULL) {
			ESP_LOGE(pcTaskGetName(0), "Failed to open define file for reading");
			ESP_LOGE(pcTaskGetName(0), "Please make Display.def");
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
		ESP_LOGI(pcTaskGetName(0), "line=[%s]", line);
		if (strlen(line) == 0) continue;
		if (line[0] == '#') continue;

		int ret = parseLine(line, 10, 32, result);
		ESP_LOGI(TAG, "parseLine=%d", ret);
		for(int i=0;i<ret;i++) ESP_LOGI(TAG, "result[%d]=[%s]", i, &result[i][0]);
		display[readLine].enable = true;
		//strlcpy(display[readLine].display_text, &result[0][0], maxText);
		strlcpy(display[readLine].display_text, &result[0][0], maxText+1);
		display[readLine].ir_cmd = strtol(&result[1][0], NULL, 16);
		display[readLine].ir_addr = strtol(&result[2][0], NULL, 16);

		readLine++;
		if (readLine == maxLine) break;
	}
	fclose(f);
	return readLine;
}

void tft(void *pvParameters)
{
	// Setup IR transmitter
	rmt_item32_t *items = NULL;
	size_t length = 0;
	ir_builder_t *ir_builder = NULL;
	rmt_config_t rmt_tx_config = RMT_DEFAULT_CONFIG_TX(RMT_TX_GPIO_NUM, RMT_TX_CHANNEL);
	rmt_tx_config.tx_config.carrier_en = true;
	rmt_config(&rmt_tx_config);
	rmt_driver_install(RMT_TX_CHANNEL, 0, 0);
	ir_builder_config_t ir_builder_config = IR_BUILDER_DEFAULT_CONFIG((ir_dev_t)RMT_TX_CHANNEL);
	ir_builder_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT;
	ir_builder = ir_builder_rmt_new_nec(&ir_builder_config);
	ESP_LOGI(pcTaskGetName(0), "Setup IR transmitter done");

	// Read display information
	DISPLAY_t display[MAX_CONFIG];
	for(int i=0;i<MAX_CONFIG;i++) display[i].enable = false;
	int readLine = readDefineFile(display, MAX_CONFIG, MAX_CHARACTER);
	if (readLine == 0) {
		while(1) { vTaskDelay(1); }
	}
	for(int i=0;i<readLine;i++) {
		ESP_LOGI(pcTaskGetName(0), "display[%d].display_text=[%s]",i, display[i].display_text);
		ESP_LOGI(pcTaskGetName(0), "display[%d].ir_cmd=[0x%02x]",i, display[i].ir_cmd);
		ESP_LOGI(pcTaskGetName(0), "display[%d].ir_addr=[0x%02x]",i, display[i].ir_addr);
	}

	int selected = 0;
	CMD_t cmdBuf;
	while(1) {
		xQueueReceive(xQueueCmd, &cmdBuf, portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(0),"cmdBuf.command=%d", cmdBuf.command);
		if (cmdBuf.command == CMD_SELECT) {
			ESP_LOGI(pcTaskGetName(0), "selected=%d",selected);
			ESP_LOGI(pcTaskGetName(0), "ir_cmd=0x%02x",display[selected].ir_cmd);
			ESP_LOGI(pcTaskGetName(0), "ir_addr=0x%02x",display[selected].ir_addr);
			uint16_t cmd = display[selected].ir_cmd;
			uint16_t addr = display[selected].ir_addr;
			cmd = ((~cmd) << 8) |  cmd; // Reverse cmd + cmd
			addr = ((~addr) << 8) | addr; // Reverse addr + addr
			ESP_LOGI(pcTaskGetName(0), "cmd=0x%x",cmd);
			ESP_LOGI(pcTaskGetName(0), "addr=0x%x",addr);

			// Send new key code
			ESP_ERROR_CHECK(ir_builder->build_frame(ir_builder, addr, cmd));
			ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
			//To send data according to the waveform items.
			rmt_write_items(RMT_TX_CHANNEL, items, length, false);
			// Send repeat code
			vTaskDelay(pdMS_TO_TICKS(ir_builder->repeat_period_ms));
			ESP_ERROR_CHECK(ir_builder->build_repeat_frame(ir_builder));
			ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
			rmt_write_items(RMT_TX_CHANNEL, items, length, false);

			if (selected == 0) {
				selected = 1;
			} else {
				selected = 0;
			}
		}
	} // end while

	// nerver reach here
	ir_builder->del(ir_builder);
	rmt_driver_uninstall(RMT_TX_CHANNEL);
	vTaskDelete(NULL);
}

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 10,
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


	xTaskCreate(tft, "TFT", 1024*4, NULL, 2, NULL);

	xTaskCreate(buttonStick, "BUTTON", 1024*4, NULL, 2, NULL);
}

