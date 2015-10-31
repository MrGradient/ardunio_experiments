#define SOUT   12
#define CLK    13
#define LD      2

void setup() {
  noInterrupts();
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("Setup begin");
  
  pinMode(SOUT, INPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LD, OUTPUT);
  Serial.println("Pin mode set done");

  digitalWrite(CLK, HIGH);
  digitalWrite(LD, HIGH);
  Serial.println("Pin level set");
  Serial.println("Setup done");
  
  interrupts();
  Serial.println("Go");  
}

void pulse(int pin){
  digitalWrite(pin, LOW);
  digitalWrite(pin, HIGH);
}

void loop() {

  
  pulse(LD);
  
  Serial.println("----");
  uint8_t data = 0x00;
  for(uint8_t i = 0; i < 8; i++){
    uint8_t in = digitalRead(SOUT);
    Serial.print(in);
    data |= (in << i);
    pulse(CLK);
  }
  Serial.println();
  Serial.println(data, BIN);
  delay(1000);

}
