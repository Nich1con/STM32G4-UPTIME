#ifndef INC_STM32G4_UPTIME_H_
#define INC_STM32G4_UPTIME_H_

#include "main.h"

/*
 * Библиотека реализующая Arduino-подобные функции realtime:
 * - Подходит для микроконтроллеров STM32G4xx
 * - Использует HSI16 в качестве источника тактирования
 * - Основана на LPTIM1, использует прерывания с частотой 1 мс
 * - Разрешение: 1 мс & 1 мкс
 * - Точность зависит от точности HSI16, в пределах +-2%
 * - За счёт HSI16, не зависит от частоты тактирования CPU
 *
 * API:
 * 	UPTIME_Init();		// Инициализация таймера и прерываний
 * 	UPTIME_DeInit();	// Полный сброс и отключение таймера
 * 	UPTIME_Reset();		// Аппаратный сброс таймера и счетчика millis()/micros()
 *  UPTIME_Suspend();	// Приостановка таймера (без сброса)
 *  UPTIME_Resume();	// Возобновление таймера
 *
 *  millis();			// Аптайм в мс
 *  micros();			// Аптайм в мкс
 *  delayMs(x);			// Задержка в мс
 *  delayUs(x);			// Задержка в мкс
*/


#define LPTIM_ARR_VALUE (1000 - 1)
volatile uint32_t _uptime_ms_cnt = 0;

/*
 *  Таймер использует HSI16 и делитель /16
 *  Частота тиков таймера = 1МГц
 *  Полный период таймера = 1мс
 *  Время 1 тика таймера = 1 мкс
 *  Тиков в периоде = 1000 (0...999)
*/

void UPTIME_Init(void);
void UPTIME_DeInit(void);
void UPTIME_Reset(void);
void UPTIME_Suspend(void);
void UPTIME_Resume(void);

uint32_t millis(void);
uint32_t micros(void);
void delayMs(uint32_t ms);
void delayUs(uint32_t us);

/* =========================== Системное ============================== */
void UPTIME_Init(void){
	UPTIME_Reset();

	if(!(RCC -> CR & RCC_CR_HSION)){
		RCC -> CR |= RCC_CR_HSION;
		while(!(RCC -> CR & RCC_CR_HSIRDY));
	}

	MODIFY_REG(RCC -> CCIPR, RCC_CCIPR_LPTIM1SEL, RCC_CCIPR_LPTIM1SEL_1);
	RCC -> APB1ENR1 |= RCC_APB1ENR1_LPTIM1EN;

	LPTIM1 -> CFGR = LPTIM_CFGR_PRESC_2;
	LPTIM1 -> IER = LPTIM_IER_ARRMIE;
	LPTIM1 -> CR = LPTIM_CR_ENABLE;
	LPTIM1 -> ARR = LPTIM_ARR_VALUE;
	LPTIM1 -> CR |= LPTIM_CR_CNTSTRT;

	NVIC_EnableIRQ(LPTIM1_IRQn);
}

void UPTIME_DeInit(void){
	NVIC_DisableIRQ(LPTIM1_IRQn);
	RCC -> APB1ENR1 &= ~RCC_APB1ENR1_LPTIM1EN;
	UPTIME_Reset();
}

void UPTIME_Reset(void){
	_uptime_ms_cnt = 0;
	RCC -> APB1RSTR1 |= RCC_APB1RSTR1_LPTIM1RST;
	RCC -> APB1RSTR1 &= ~RCC_APB1RSTR1_LPTIM1RST;
}

void UPTIME_Suspend(void){
	LPTIM1 -> CR &= ~LPTIM_CR_ENABLE;
}

void UPTIME_Resume(void){
	LPTIM1 -> CR |= LPTIM_CR_ENABLE;
	LPTIM1 -> CR |= LPTIM_CR_CNTSTRT;
}


/* =========================== Основное ============================== */
uint32_t millis(void){
	return _uptime_ms_cnt;
}

uint32_t micros(void){
	return ((_uptime_ms_cnt * 1000UL) + LPTIM1 -> CNT);
}

void delayMs(uint32_t ms){
	uint32_t start = millis();
	while(millis() - start < ms);
}

void delayUs(uint32_t us){
	uint32_t start = micros();
	while(micros() - start < us);
}

#ifdef __cplusplus
extern "C" {
#endif

void LPTIM1_IRQHandler () {
	LPTIM1 -> ICR = (LPTIM_ICR_ARRMCF | LPTIM_ICR_CMPMCF);
	_uptime_ms_cnt++;
}

#ifdef __cplusplus
}
#endif

#endif
