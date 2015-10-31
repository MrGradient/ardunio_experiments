#define SOUT   12
#define CLK    13
#define LD      2

void setup() {
  noInterrupts();
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Setup begin");

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  pinMode(SOUT, INPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LD, OUTPUT);
  Serial.println("Pin mode set done");
  
  digitalWrite(LD, HIGH);
  Serial.println("Pin level set");

  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR = (1 << SPI2X);
  Serial.println("Setup done");
  
  interrupts();
  Serial.println("Go");  
}

void pulse(int pin){
  digitalWrite(pin, LOW);
  digitalWrite(pin, HIGH);
}

void loop() {

  Serial.println("----");
  pulse(LD);
  SPDR = 0;
  while (!(SPSR & (1<<SPIF)));
  uint8_t data = SPDR; 
  Serial.println(data, BIN);

  delay(1000);
}
