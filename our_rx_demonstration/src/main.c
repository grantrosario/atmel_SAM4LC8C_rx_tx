
#include <asf.h>
#include "config.h"
#include "phy.h"
#include "nwk.h"
#include "sys.h"
#include "sysTimer.h"

typedef struct {
	uint32_t startDelimeter;
	uint16_t length;
	uint16_t sequenceNumber;
	uint16_t temperature;
	uint16_t light;
	uint16_t lqi;
	uint16_t crc;
	uint32_t endDelimeter;
} message_t;

static void appInit(void);
static bool rxInd(NWK_DataInd_t *ind);

static void appInit(void) {
	NWK_SetAddr(APP_ADDR);
	NWK_SetPanId(APP_PANID);
	PHY_SetChannel(APP_CHANNEL);
	PHY_SetRxState(true);
	
	NWK_OpenEndpoint(APP_ENDPOINT, rxInd);
}


static bool rxInd(NWK_DataInd_t *ind) {
	uint8_t index;
		
	if(ind->size == sizeof(message_t)) {
		message_t* data;
		data = (message_t*) ind->data;
		data->lqi = ind->lqi;
// 		printf("sequence number=%d\r\n", data->sequenceNumber);
// 		printf("temperature=%d\r\n", data->temperature);
// 		printf("light=%d\r\n", data->light);
// 		printf("lqi=%d or %d\r\n", ind->lqi, data->lqi);
		usart_serial_write_packet(USART1, ind->data, ind->size);
	}
		
	return (true);
}

int main (void) {
	
	sysclk_init();
	board_init();
	SYS_Init();
	
	appInit();

	usart_serial_options_t serial_config = {
		.baudrate = 9600,
		.charlength = US_MR_CHRL_8_BIT,
		.paritytype = US_MR_PAR_NO,
		.stopbits = US_MR_NBSTOP_1
	};
	usart_serial_init(USART1, &serial_config);

	// setup stdio
	stdio_base = USART1;
	ptr_put = (int(*)(void volatile*, char)) usart_serial_putchar;
	setbuf(stdout, NULL);

	while (1) {
		SYS_TaskHandler();
	}
}
