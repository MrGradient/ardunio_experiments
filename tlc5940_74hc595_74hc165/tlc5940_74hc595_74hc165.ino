#define N_TLC  1
#define ROWS 6

#define SCLK  13

// tlc
#define SIN   11
#define VPRG   5
#define XLAT   6
#define BLANK  4
#define DCPRG  3
#define GSCLK  9

// 595
#define RCK    7 

// 165
#define SOUT   12
#define LD      2

#define R_OFF 0

uint16_t wait = 0;
uint16_t hue = 0;
volatile int8_t up = 0;

volatile uint8_t gsUpdateFlag;
volatile uint8_t row = 0;

const int DC = B00111111;

int dcData[12 * N_TLC] = {0};
int gsData[8][24 * N_TLC] = {0};

uint16_t red = 0;
uint16_t green = 0;
uint16_t blue = 0; 

uint16_t colors[ROWS][3] = {0};

uint8_t MAX = 12;

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
  pinMode(SOUT, INPUT);
  pinMode(LD, OUTPUT);

  pinMode(RCK, OUTPUT);
  Serial.println("Pin mode set done");

  digitalWrite(GSCLK, LOW);
  digitalWrite(SCLK, LOW);
  digitalWrite(DCPRG, LOW);
  digitalWrite(VPRG, HIGH);
  digitalWrite(XLAT, LOW);
  digitalWrite(BLANK, HIGH);
  digitalWrite(RCK, LOW);
  digitalWrite(LD, HIGH);
  
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

  // Timer 2
  TCCR2A |= (1 << WGM21); // Configure timer 2 for CTC mode
  TCCR2B |= ((1 << CS22) | (1 << CS21) | (1 << CS22)); // Start timer at Fcpu/64
  TIMSK2 |= (1 << OCIE2A); // Enable CTC interrupt
  OCR2A   = 249; 

  interrupts();
  clockInDotCorrection();
}

void pulse(int pin){
  digitalWrite(pin, HIGH);
  digitalWrite(pin, LOW);
}

void pulseInv(int pin){
  digitalWrite(pin, LOW);
  digitalWrite(pin, HIGH);
}

static inline void setGsUpdateFlag(void) {
  __asm__ volatile ("" ::: "memory");
  gsUpdateFlag = 1;
}

void clockInDotCorrection() {

  digitalWrite(DCPRG, HIGH);
  digitalWrite(VPRG, HIGH);
  
  setDc(4, 0b00111111); // rot
  setDc(5, 0b00111111); // gruen
  setDc(6, 0b00111111); // blau

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

ISR(TIMER2_COMPA_vect) {

  pulseInv(LD);
  SPDR = 0x00;
  while (!(SPSR & (1 << SPIF))){};
  uint8_t data = SPDR;
  
  while(gsUpdateFlag);

  for(int i = 0; i < 6; i++){
    uint8_t a = (data >> i) & 0x01;
    
    if(a == 1){
      setGs(i + R_OFF, 4, colors[i][0] );
      setGs(i + R_OFF, 5, colors[i][1] );
      setGs(i + R_OFF, 6, colors[i][2] );
    }else {
      setGs(i + R_OFF, 4, 0);
      setGs(i + R_OFF, 5, 0);
      setGs(i + R_OFF, 6, 0);
    }
  }
  if(wait < 50){
    wait ++;
  } else{
    wait = 0;
    up = 1;
  }
}

void selectRow(uint8_t r){
  digitalWrite(RCK, LOW);
  SPDR = (1 << r);
  while (!(SPSR & (1 << SPIF))){};
  digitalWrite(RCK, HIGH);
}

void setDc(uint8_t channel, uint8_t value) {
  channel = (16 * N_TLC) - 1 - channel;
  uint16_t i = (uint16_t)channel * 3 / 4;
  switch (channel % 4) {
    case 0:
      dcData[i] = (dcData[i] & 0x03) | (uint8_t)(value << 2);
      break;
    case 1:
      dcData[i] = (dcData[i] & 0xFC) | (value >> 4);
      i++;
      dcData[i] = (dcData[i] & 0x0F) | (uint8_t)(value << 4);
      break;
    case 2:
      dcData[i] = (dcData[i] & 0xF0) | (value >> 2);
      i++;
      dcData[i] = (dcData[i] & 0x3F) | (uint8_t)(value << 6);
      break;
    default: // case 3:
      dcData[i] = (dcData[i] & 0xC0) | (value);
      break;
  }
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

void hsi2rgb(float H, float S, float I, int* rgb) {
  int r, g, b;
  H = fmod(H,360); // cycle H around to 0-360 degrees
  H = 3.14159*H/(float)180; // Convert to radians.
  S = S>0?(S<1?S:1):0; // clamp S and I to interval [0,1]
  I = I>0?(I<1?I:1):0;
    
  // Math! Thanks in part to Kyle Miller.
  if(H < 2.09439) {
    r = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    g = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    b = 255*I/3*(1-S);
  } else if(H < 4.188787) {
    H = H - 2.09439;
    g = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    b = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    r = 255*I/3*(1-S);
  } else {
    H = H - 4.188787;
    b = 255*I/3*(1+S*cos(H)/cos(1.047196667-H));
    r = 255*I/3*(1+S*(1-cos(H)/cos(1.047196667-H)));
    g = 255*I/3*(1-S);
  }
  rgb[0]=r;
  rgb[1]=g;
  rgb[2]=b;
}

void loop (){

  while(up != 1);

  for(uint8_t i = 0; i < ROWS - 1; i++){
    colors[i][0] = colors[i+1][0];
    colors[i][1] = colors[i+1][1];
    colors[i][2] = colors[i+1][2];
  }

  int rgb[3];
  hsi2rgb(hue, 1.0F, 1.0F, rgb);
  //hsi2rgb(0.0F, 0.0F, 1.0F, rgb);

  colors[5][0] = rgb[0];
  colors[5][1] = rgb[1];
  colors[5][2] = rgb[2];
  hue+=6;
  if(hue > 360){
    hue = 0;
  }
  up = 0;
}


