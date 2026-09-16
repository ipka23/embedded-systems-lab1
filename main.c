#include "main.h"
#include "tm1637.h"

uint16_t counter;
volatile uint32_t tickCount;
uint32_t last_display_update;
int16_t firstOperand;
int16_t secondOperand;
int16_t result;
uint16_t inputStage;
char lastKey;
uint16_t operation;
uint32_t lastScanTime;
uint16_t operationNumber;

void osSystickHandler(void) {
  tickCount++;
}


void initGPIO() {
  // Включаем тактирование GPIOA и GPIOB
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;

  // Настраиваем PA5 как выход
  GPIOA->MODER = (GPIOA->MODER & ~(3 << 10)) | (1 << 10);
  GPIOA->OTYPER &= ~(1 << 5);
  GPIOA->OSPEEDR |= (1 << 10);
}

void initUSART2() {
  // Включаем тактирование USART2
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

  // Настраиваем PA2 и PA3 в альтернативный режим
  GPIOA->MODER = (GPIOA->MODER & ~(0xF << 4)) | (0xA << 4);
  GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xFF << 8)) | (1 << 8) | (1 << 12);

  // Настраиваем USART2
  USART2->BRR = 417; // 48MHz/115200
  USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void initSysTick() {
  SysTick->LOAD = 47999; // 1ms при 48MHz
  SysTick->VAL = 0;
  SysTick->CTRL = (1 << 2) | (1 << 1) | (1 << 0);
}

int _write(int file, uint8_t *ptr, int len) {
  for (int i = 0; i < len; i++) {
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = ptr[i];
  }
  return len;
}

void checkTickCount() {
  if ((tickCount % 2000) == 0) {
    GPIOA->ODR ^= (1 << 5); // Toggle LED
    printf("tickCount = %d!\n", tickCount++);
  }
}

void calculator(char key) {
  if (inputStage == 0 && key >= '0' && key <= '9') {
      firstOperand = key - '0';
      tm1637_display_number(firstOperand);
      inputStage = 1;
  }
  else if ((inputStage == 1 || inputStage == 2) && key == '*') {
    switch (operationNumber) {
      case 1:
        printf("Operation: +\n");
        break;
      case 2:
        printf("Operation: -\n");
        break;
      case 3:
        printf("Operation: *\n");
        break;
      case 4:
        printf("Operation: /\n");
        break;
    }
    operation = operationNumber;
    operationNumber++;
    if (operationNumber > 4) {
      operationNumber = 1;
    }
    inputStage = 2;
  } else if (inputStage == 2 && key >= '0' && key <= '9') {
    secondOperand = key - '0';
    tm1637_display_number(secondOperand);
    inputStage = 3;
  } else if (key == '#' && inputStage == 3) {
    if (operationNumber != 0 && firstOperand != -1 && secondOperand != -1) {
      switch (operation) {
        case 1:
          result = firstOperand + secondOperand; 
          break;
        case 2:
          result = firstOperand - secondOperand; 
          break;
        case 3:
          result = firstOperand * secondOperand; 
          break;
        case 4:
          result = firstOperand / secondOperand; 
          break;
      }
      if (result < 0) {
        tm1637_display_number(result * -1);
        tm1637_display_digit(2, 0x40);
      } else {
        tm1637_display_number(result);
      }
      
      firstOperand = -1;
      secondOperand = -1;
      operationNumber = 1;
      inputStage = 0;
    }
  } 
}

int main(void) {
  initGPIO();
  initUSART2();
  initSysTick();
  initKeyboard();
  tm1637_init();

  operationNumber = 1;
  firstOperand = -1;
  secondOperand = -1;
  inputStage = 0;

  while (1) {
    char key = scanKeyboard();

    calculator(key);
  }

  return 0;
}