
#define IOPIN_LED_FAN            32
#define IOPIN_LED_HEATER         33
#define IOPIN_LED_CONNECTIVITY   25
#define IOPIN_HEATER             26
#define IOPIN_FAN                27



void setup() {
  // put your setup code here, to run once:
   pinMode(IOPIN_LED_FAN, OUTPUT);
   pinMode(IOPIN_LED_HEATER, OUTPUT);
   pinMode(IOPIN_LED_CONNECTIVITY, OUTPUT);
   pinMode(IOPIN_HEATER, OUTPUT);
   pinMode(IOPIN_FAN, OUTPUT);

   /*
   digitalWrite(IOPIN_LED_FAN, HIGH);
   digitalWrite(IOPIN_LED_HEATER, HIGH);
   digitalWrite(IOPIN_LED_CONNECTIVITY, HIGH);
   digitalWrite(IOPIN_HEATER, HIGH);
   digitalWrite(IOPIN_FAN, HIGH);
   */
}

void loop() {
  digitalWrite(IOPIN_LED_HEATER, HIGH);
  digitalWrite(IOPIN_HEATER, HIGH);
  delay(1000);
  digitalWrite(IOPIN_LED_HEATER, LOW);
  digitalWrite(IOPIN_HEATER, LOW);
  delay(1000);
}
