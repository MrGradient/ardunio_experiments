#include <SPI.h>

#define N_TLC  2

#define SIN   11
#define SCLK  13
#define VPRG   5
#define XLAT   6
#define BLANK  4
#define DCPRG  3
#define GSCLK  9

const uint8_t DC = B00111111;

volatile uint8_t gsUpdateFlag;

int dcData[12 * N_TLC] = {0};
int gsData[24 * N_TLC] = {0};

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Setup begin");
  
  pinMode(SIN, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(VPRG, OUTPUT);
  pinMode(XLAT, OUTPUT);
  pinMode(BLANK, OUTPUT);
  pinMode(DCPRG, OUTPUT);
  pinMode(GSCLK, OUTPUT);
  Serial.println("Pin mode set done");

  digitalWrite(GSCLK, LOW);
  digitalWrite(SCLK, LOW);
  digitalWrite(DCPRG, LOW);
  digitalWrite(VPRG, HIGH);
  digitalWrite(XLAT, LOW);
  digitalWrite(BLANK, HIGH);
  Serial.println("Pin level set");

  SPI.beginTransaction(SPISettings(SPI_CLOCK_DIV2, MSBFIRST, SPI_MODE0));
  SPI.begin();
  Serial.println("Setup done");

  clockInDotCorrection();

  noInterrupts();
  TCCR0A = (1 << WGM01);
  TCCR0B = ((1 << CS02) | (1 << CS00));
  OCR0A = 254;
  TIMSK0 |= (1 << OCIE0A);

  TCCR1B |= (1 << CS11); 
  TCCR1A |= ((1<<WGM11) | (1<<WGM10));
  TCCR1B |= ((1<<WGM13) | (1<<WGM12));
  TCCR1A |= (1 << COM1A0);
  TCNT1 = 0;

  interrupts();
  Serial.println("Go");
}

void pulse(int pin){
  digitalWrite(pin,HIGH);
  digitalWrite(pin,LOW);
}

static inline void setGsUpdateFlag(void) {
  __asm__ volatile ("" ::: "memory");
  gsUpdateFlag = 1;
}

void clockInDotCorrection() {

  digitalWrite(DCPRG, HIGH);
  digitalWrite(VPRG, HIGH);

  setAllDc(DC);
  for(int i = 0; i < 12 * N_TLC; i++){
    SPI.transfer(dcData[i]);
  }
  
  Serial.println("Dot correction sent");
  pulse(XLAT);
}

int xlatNeedsPulse = 0;

ISR(TIMER0_COMPA_vect) {

  digitalWrite(BLANK, HIGH);

  if(digitalRead(VPRG) == 1) {
    digitalWrite(VPRG, LOW);
    if(xlatNeedsPulse == 1){
      pulse(XLAT);
      xlatNeedsPulse = 0;
    }
    
    } else if (xlatNeedsPulse == 1) {
      pulse(XLAT);
      xlatNeedsPulse = 0;
    }
    
    digitalWrite(BLANK, LOW);

    if(gsUpdateFlag == 1){
      for(int i = 0; i < N_TLC * 24; i++){
        SPI.transfer(gsData[i]);
      }
      xlatNeedsPulse = 1;
      gsUpdateFlag = 0;
    }
    
    
  }

void setAllDc(uint8_t value) {
  uint8_t tmp1 = (uint8_t)(value << 2);
  uint8_t tmp2 = (uint8_t)(tmp1 << 2);
  uint8_t tmp3 = (uint8_t)(tmp2 << 2);
  tmp1 |= (value >> 4);
  tmp2 |= (value >> 2);
  tmp3 |= value;
  uint8_t i = 0;
  do {
    dcData[i++] = tmp1; // bits: 05 04 03 02 01 00 05 04
    dcData[i++] = tmp2; // bits: 03 02 01 00 05 04 03 02
    dcData[i++] = tmp3; // bits: 01 00 05 04 03 02 01 00
  } while (i < 12 * N_TLC);
} 

void setGs(int channel, uint16_t value) {
  channel = (16 * N_TLC) - 1 - channel;
  uint16_t i = (uint16_t)channel * 3 / 2;
  switch (channel % 2) {
    case 0:
      gsData[i] = (value >> 4);
      i++;
      gsData[i] = (gsData[i] & 0x0F) | (uint8_t)(value << 4);
      break;
    default: // case 1:
      gsData[i] = (gsData[i] & 0xF0) | (value >> 8);
      i++;
      gsData[i] = (uint8_t)value;
    break;
  }
}

int16_t count = 0;
uint16_t level [16 * N_TLC] = {
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 11, 11, 11, 11, 12, 11, 11, 11, 11, 10, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1
};

long brightness = 0L;
int8_t factor = 10;

void loop (){

  if(count < 20000){
    count++;
    return;
  } else {
      count = 0;
  }
    
  int tmp = level[0];
  for(int i = 0; i < (16 * N_TLC - 1); i++){
    level[i] = level[i+1];
  }
  level[16 * N_TLC - 1] = tmp;

//  if(brightness >= 4095){
//    factor = abs(factor) * -1;
//  } else if(brightness <= 0){
//    factor = abs(factor);
//  }


//  if(brightness < 0) {
//    brightness = 0;
//    Serial.println("Min");
//  }
//  if(brightness > 4095) {
//    brightness = 4095;
//    Serial.println("Max");
//  }

  while(gsUpdateFlag);
  for(int i = 0; i < 16 * N_TLC; i++){
    setGs(i, (1L << level[i])-1);
    //setGs(i, (uint16_t)brightness);
  }
  setGsUpdateFlag();
}






