#include "Rand_lib.h"
volatile int wait;
volatile int callback_result;

int RandomInt(void){
    return MXC_TRNG_RandomInt();
}

void Rand_NASYC(uint8_t *buf, uint32_t len){
    memset(buf, 0, len);
    MXC_TRNG_Init();
    MXC_TRNG_Random(buf, len);
    MXC_TRNG_Shutdown();
}
void Rand_ASYC(uint8_t *data, uint32_t len){
    MXC_TRNG_Init();
    wait = 1;
    NVIC_EnableIRQ(TRNG_IRQn);
    MXC_TRNG_RandomAsync(data, len, &Test_Callback);
    while (wait) {
        continue;
    }
    MXC_TRNG_Shutdown();
}


void TRNG_IRQHandler(void)
{
    MXC_TRNG_Handler();
}

void Test_Callback(void *req, int result)
{
    wait = 0;
    callback_result = result;
}