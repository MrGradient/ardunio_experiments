#define N_TLC  1
#define ROWS 6

// tlc
#define SIN   11
#define SCLK  13
#define VPRG   5
#define XLAT   6
#define BLANK  4
#define DCPRG  3
#define GSCLK  9

// 595
#define RCK    7 

volatile uint8_t gsUpdateFlag;
volatile uint8_t row = 2;

const int DC = B00111111;

int dcData[12 * N_TLC] = {0};
int gsData[8][24 * N_TLC] = {0};

void setup() {
  noInterrupts();
  
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Setup begin");

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  pinMode(SIN, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(VPRG, OUTPUT);
  pinMode(XLAT, OUTPUT);
  pinMode(BLANK, OUTPUT);
  pinMode(DCPRG, OUTPUT);
  pinMode(GSCLK, OUTPUT);

  pinMode(RCK, OUTPUT);
  Serial.println("Pin mode set done");

  digitalWrite(GSCLK, LOW);
  digitalWrite(SCLK, LOW);
  digitalWrite(DCPRG, LOW);
  digitalWrite(VPRG, HIGH);
  digitalWrite(XLAT, LOW);
  digitalWrite(BLANK, HIGH);
  digitalWrite(RCK, LOW);
  
  // SPI
  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR = (1 << SPI2X);
  Serial.println("Setup done");

  // Timer 0
  TCCR0A = (1 << WGM01);
  TCCR0B = (1 << CS02);
  OCR0A = 31;
  TIMSK0 |= (1 << OCIE0A);

  // Timer 1 - 4MHz, Duty cycle 50%
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1A |= (1 << WGM11);
  TCCR1B |= ((1 << WGM13) | (1 << CS10));
  OCR1A = 1;
  ICR1 = 2;
  TCCR1A |= (1 << COM1A1);
  
  interrupts();
  clockInDotCorrection();
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
    SPDR = dcData[i];
    while (!(SPSR & (1 << SPIF))){};
  }
  
  Serial.println("Dot correction sent");
  pulse(XLAT);
}

int xlatNeedsPulse = 0;
ISR(TIMER0_COMPA_vect) {
  digitalWrite(BLANK, HIGH);
  
  if(digitalRead(VPRG) == 1) {
    digitalWrite(VPRG, LOW);
  } 
  
  if (xlatNeedsPulse == 1) {
    pulse(XLAT);    
    xlatNeedsPulse = 0;
    selectRow(row);
    row++;
    if(row == 8) row = 0;
  }
  
  digitalWrite(BLANK, LOW);

  if(1 == 1){  
    for(int i = 0; i < N_TLC * 24; i++){
      SPDR = gsData[row][i];
      while (!(SPSR & (1 << SPIF)));
    }
    xlatNeedsPulse = 1;
    gsUpdateFlag = 0;
  }
} 

void selectRow(uint8_t r){
  digitalWrite(RCK, LOW);
  SPDR = (1 << r);
  while (!(SPSR & (1 << SPIF))){};
  digitalWrite(RCK, HIGH);
}
1
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


void setGs(int row, int channel, uint16_t value) {
  channel = (16 * N_TLC) - 1 - channel;
  uint16_t i = (uint16_t)channel * 3 / 2;
  switch (channel % 2) {
    case 0:
      gsData[row][i] = (value >> 4);
      i++;
      gsData[row][i] = (gsData[row][i] & 0x0F) | (uint8_t)(value << 4);
      break;
    default: // case 1:
      gsData[row][i] = (gsData[row][i] & 0xF0) | (value >> 8);
      i++;
      gsData[row][i] = (uint8_t)value;
    break;
  }
}

int16_t count = 0;
uint16_t level [12] = {
  0, 3, 5, 8, 10, 11, 12, 11, 10, 8, 6, 3
  //3, 6, 12, 12,  6, 3, 3, 6, 12, 12,  6, 3, 
 // 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 0,
};

int brightness = 0;
int8_t factor = 10;

#define R_OFF 2

void loop (){
  
  if(count < 20000){
    count++;
    return;
  } else {
    count = 1;
  }

  int tmp = level[0];
  for(int i = 0; i < 12; i++){
    level[i] = level[i+1];
  }
  level[12 - 1] = tmp;
  

  while(gsUpdateFlag);
  for(int i = R_OFF; i < ROWS + R_OFF; i++){
    
    setGs(i, 4, (1L << level[i - R_OFF]) - 1);
    setGs(i, 5, (1L << level[11 - (i - R_OFF)]) - 1);

    //setGs(i, brightness);
  }
  setGsUpdateFlag();
}
