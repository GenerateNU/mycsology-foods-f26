//define 2 distinct memory addreses in SRAM for storing the data
//CAN DELETE THIS LATER, NOT BEING USED
#define SRAM_ADDR_TEMP ((volatile uint8_t *)0x20001000)
#define SRAM_ADDR_HUM ((volatile uint8_t *)0x20001004)

void ProcessData(void){

    volatile uint8_t *ptr_temp = SRAM_ADDR_TEMP;
    volatile uint8_t *ptr_hum = SRAM_ADDR_HUM;

    uint8_t temp = *ptr_temp;
    uint8_t hum = *ptr_hum;

}