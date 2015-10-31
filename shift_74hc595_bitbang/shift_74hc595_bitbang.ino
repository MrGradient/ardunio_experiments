
#define SIN   11
#define SCLK  13
#define RCK    7 

void setup() {
  noInterrupts();
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Setup begin");

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  pinMode(SIN, OUTPUT);
  pinMode(SCLK, OUTPUT);
  pinMode(RCK, OUTPUT);
  Serial.println("Pin mode set done");

  digitalWrite(SIN, LOW);
  digitalWrite(SCLK, LOW);
  digitalWrite(RCK, LOW);
  Serial.println("Pin level set");
 
  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR = (1 << SPI2X);
  Serial.println("Setup done");
  
  interrupts();
  Serial.println("Go");  
}

int shift = 0;
int factor = 1;

uint16_t c = 0;
void loop() {

  if(c < 60000){
    c++;
    return;
  } 
  c = 0;

  if(shift == 0){
    factor = 1;
  }
  if(shift == 7){
    factor = -1;
  }

  shift = shift + factor;

  digitalWrite(RCK, LOW);
  SPDR = (1 << shift);
  while (!(SPSR & (1 << SPIF))){};
  digitalWrite(RCK, HIGH);
}
