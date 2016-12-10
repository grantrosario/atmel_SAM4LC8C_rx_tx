#include <asf.h>
#include <string.h>
#include "config.h"
#include "phy.h"
#include "nwk.h"
#include "sys.h"
#include "sysTimer.h"
#include "at30tse75x.h"

typedef struct adc_dev_inst adc_dev_inst_t;
typedef struct adc_config adc_config_t;
typedef struct adc_seq_config adc_seq_config_t;
typedef struct adc_ch_config adc_ch_config_t;

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
static void timerHandler(SYS_Timer_t *timer);
static void sendData(void);
static void sendDataConf(NWK_DataReq_t *request);
static void calculateCRC(uint8_t* data, uint8_t length, uint8_t* storage);

void checkLightData(void);
void configure_adc(void);
void start_timer(void);
void adc_conversion_complete(void);
void readTempData(void);

static SYS_Timer_t txTimer;
static bool radioBusy = false;
static message_t payload;
static NWK_DataReq_t txRequest;
static adc_dev_inst_t adc0;
static uint16_t light_value = 0xff;
static double temp_value;
static bool conversion_complete = true;

void adc_conversion_complete(void) {
	
	if(adc_get_status(&adc0) & ADCIFE_SR_SEOC) {
		
		// store result in our global variable
		light_value = adc_get_last_conv_value(&adc0);
		conversion_complete = true;
		adc_clear_status(&adc0, ADCIFE_SCR_SEOC);
	}
	
}

void configure_adc(void) {
	adc_config_t adcConfig = {
		.clksel = ADC_CLKSEL_APBCLK,		// drive the ADC from the bus clock
		.prescal = ADC_PRESCAL_DIV512,		// prescaler of 512 (slowest sample and hold)
		.speed = ADC_SPEED_75K,				// slowest sampling speed (for current consumption)
		.refsel = ADC_REFSEL_0,				// internal 1.0V Vref
		.start_up = CONFIG_ADC_STARTUP		// startup penalty (in clock cycles)
	};
	
	adc_seq_config_t adcSequencerConfig = {
		.bipolar = ADC_BIPOLAR_SINGLEENDED,	// single-ended input
		.res = ADC_RES_12_BIT,				// 12-bit resolution
		.zoomrange = ADC_ZOOMRANGE_0,		// no zooming; (interesting feature!)
		.muxpos = ADC_MUXPOS_2,				// AD0
		.muxneg = ADC_MUXNEG_1,				// ground reference from pad
		.internal = ADC_INTERNAL_2,			// (implied by MUXPOS_0 and MUXNEG_1)
		.gcomp = ADC_GCOMP_DIS,				// no gain compensation
		.hwla = ADC_HWLA_DIS				// no left alignment
	};
	
	struct adc_ch_config adcChannelConfig = {
		.seq_cfg = &adcSequencerConfig,		// sequencer configuration
		.internal_timer_max_count = 0,		// timer not used
		.window_mode = ADC_WM_OFF,			// no windowing
		.low_threshold = 0,					// no low threshold
		.high_threshold = 0,				// no high threshold
	};
	
	adc_init(&adc0, ADCIFE, &adcConfig);
	adc_enable(&adc0);
	adc_ch_set_config(&adc0, &adcChannelConfig);
	adc_set_callback(&adc0, ADC_SEQ_SEOC, adc_conversion_complete, ADCIFE_IRQn, 1);
}

void checkLightData(void){
	// start a conversion on the adc
	if(conversion_complete) {
		conversion_complete = false;
		adc_start_software_conversion(&adc0);
	}
}

static void appInit(void) {
	NWK_SetAddr(APP_ADDR);
	NWK_SetPanId(APP_PANID);
	PHY_SetChannel(APP_CHANNEL);
	PHY_SetRxState(false);
	
	txTimer.interval = 1000;
	txTimer.mode = SYS_TIMER_PERIODIC_MODE;
	txTimer.handler= timerHandler;
	SYS_TimerStart(&txTimer);
}

static void timerHandler(SYS_Timer_t *timer) {
	if(timer == &txTimer) {
		if(!radioBusy) {
			sendData();
		}
	}
}

static void calculateCRC(uint8_t* data, uint8_t length, uint8_t* storage) {
	
	uint16_t result = 0;
	uint8_t index;
	for (index = 0; index < length; index++) {
		result += (data[index] & 0x0f);
		//printf("Calculating CRC: %d with index %d\r\n",result,index);
		result &= 0x0fff;
	}
	
	*storage = result;
}

static void sendData(void) {
	static uint16_t sequenceNumber = 1;
	readTempData();
	checkLightData();
	
	// create fake data
	payload.startDelimeter = 0xAABBCCDD;
	payload.length = 12;
	payload.sequenceNumber = sequenceNumber++;
	payload.temperature = temp_value*100;
	payload.light = light_value;
	payload.lqi = 257;  // Impossible value, denoting non-specified data.
	payload.crc = 0;
	calculateCRC((uint8_t*) &payload.length, 8, &payload.crc);
	// Doesn't include the CRC or LQI fields in the calculations, because
	// including the CRC value would be a circular reference, and the LQI
	// field is dynamic as the packet is sent.
	payload.endDelimeter = 0xDDCCBBAA;
	
	txRequest.dstAddr = 1;
	txRequest.dstEndpoint = APP_ENDPOINT;
	txRequest.srcEndpoint = APP_ENDPOINT;
	txRequest.options = 0;
	txRequest.data = (uint8_t*) &payload;
	txRequest.size = sizeof(payload);
	txRequest.confirm = sendDataConf;
	
	NWK_DataReq(&txRequest);
	radioBusy = true;
	printf("Luminosity: %d\r\n",light_value);
}

static void sendDataConf(NWK_DataReq_t *request) {
	if(request == &txRequest) {
		radioBusy = false;
		if(request->status == NWK_SUCCESS_STATUS) {
			ioport_toggle_pin_level(LED0);
		}
	}
}

void readTempData(void){
		at30tse_read_temperature(&temp_value);
		
		//---PRINT TEMP VALUES [TAKE OUT FOR FINAL PROGRAM]---
		char string[20];
		sprintf(string, "Temperature: %.2f C", (float) temp_value);
		printf("%s\r\n", string);
}

int main (void) {
	sysclk_init();
	board_init();
	configure_adc();
	SYS_Init();
		
	appInit();
	
	// temp sensor setup
	at30tse_init();
	at30tse_write_config_register(AT30TSE_CONFIG_RES(AT30TSE_CONFIG_RES_12_bit));
	
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
	
	while(1) {
		SYS_TaskHandler();
	}
}
