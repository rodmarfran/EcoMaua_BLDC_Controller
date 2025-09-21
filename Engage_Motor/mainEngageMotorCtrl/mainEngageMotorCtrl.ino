/**
 * Allows the interface between a control interface and the engage motor on EcoMaua vehicle
 * Use Tools >> Board >> Arduino AVR Board >> Arduino Nano
 */

#define CTRL_IN_ENGAGE_PIN 2  /* Pin 2 on Arduino Nano */
#define CTRL_OUT_ENABLE_PIN 4 /* Pin 4 on Arduino Nano */
#define PWM_OC1A_PIN 9        /* Pin 9 on Arduino Nano, OC1A */
#define PWM_OC1B_PIN 10       /* Pin 10 on Arduino Nano, OC1B */
#define STATUS_LED_PIN 13     /* Pin 13 on Arduino Nano for status LED */

/* Motor and battery voltage constants */
const float MOTOR_VOLTAGE = 12.0;  /* Default motor voltage */
const float BATTERY_VOLTAGE = 24.0;  /* Default battery voltage */

/* Duty cycle calculated based on motor and battery voltages */
const float DUTY_CYCLE = MOTOR_VOLTAGE / BATTERY_VOLTAGE;

/* Delay time in milliseconds */
const unsigned long ENGAGE_DELAY = 1000;  /* 5 seconds delay */

bool prev_engage_state = false;  /* Previous state of the engage signal */
bool current_engage_state = false;  /* Current state of the engage signal */
unsigned long lastDebounceTime = 0;  /* Last debounce time */
const unsigned long debounceDelay = 10;  /* Debounce delay time in milliseconds */

void setup() {
  /* Set the CTRL_IN pin as input with internal pull-up resistor */
  pinMode(CTRL_IN_ENGAGE_PIN, INPUT_PULLUP);

  /* Set the CTRL_OUT pin as output and initialize it to LOW */
  pinMode(CTRL_OUT_ENABLE_PIN, OUTPUT);
  digitalWrite(CTRL_OUT_ENABLE_PIN, LOW);

  /* Set the PWM pins as outputs */
  pinMode(PWM_OC1A_PIN, OUTPUT);
  pinMode(PWM_OC1B_PIN, OUTPUT);

  /* Set the status LED pin as output and initialize it to LOW */
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  /* Configure Timer1 for 20000 Hz PWM */
  configure_timer1_pwm();
}

void loop() {
  bool reading = digitalRead(CTRL_IN_ENGAGE_PIN);

  /* Debounce the engage pin */
  if (reading != prev_engage_state) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != current_engage_state) {
      current_engage_state = reading;

      /* Detect rising edge */
      if (current_engage_state) {
        engage_motor(true);
      }

      /* Detect falling edge */
      else {
        engage_motor(false);
      }
      
    }
  }

  prev_engage_state = reading;
}

/* Configure Timer1 for 20000 Hz PWM */
void configure_timer1_pwm() {
  /* Stop Timer1 */
  TCCR1A = 0;
  TCCR1B = 0;

  /* Set Timer1 to Fast PWM mode with ICR1 as top */
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12);

  /* Set prescaler to 8 */
  TCCR1B |= (1 << CS11);

  /* Set top value for 20000 Hz PWM (ICR1 value) */
  ICR1 = 399;

  /* Set initial duty cycle to 0% */
  OCR1A = 0;
  OCR1B = 0;

  /* Enable PWM on OC1A and OC1B */
  TCCR1A |= (1 << COM1A1) | (1 << COM1B1);
}

/* Engage or disengage the motor based on the engage flag */
void engage_motor(bool engage) {
  digitalWrite(CTRL_OUT_ENABLE_PIN, HIGH);
  digitalWrite(STATUS_LED_PIN, HIGH);

  if (engage) {
    /* Set OC1A to calculated duty cycle, OC1B to GND */
    set_pwm_duty_cycle_A(DUTY_CYCLE);
    set_pwm_duty_cycle_B(0.0);
  } else {
    /* Set OC1B to calculated duty cycle, OC1A to GND */
    set_pwm_duty_cycle_A(0.0);
    set_pwm_duty_cycle_B(DUTY_CYCLE);
  }

  start_timer1();
  delay(ENGAGE_DELAY); /* Wait for the defined delay time */
  stop_motor();
}

/* Stop the motor by disabling the output and resetting the PWM */
void stop_motor() {
  digitalWrite(CTRL_OUT_ENABLE_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  /* Set both OC1A and OC1B to GND */
  set_pwm_duty_cycle_A(0.0);
  set_pwm_duty_cycle_B(0.0);

  stop_timer1();
}

/* Start Timer1 with prescaler 8 */
void start_timer1() {
  TCCR1B |= (1 << CS11);
}

/* Stop Timer1 */
void stop_timer1() {
  TCCR1B &= ~(1 << CS11);
}

/* Set PWM duty cycle for channel A */
void set_pwm_duty_cycle_A(float duty_cycle) {
  duty_cycle = constrain(duty_cycle, 0.0, 1.0);
  OCR1A = (int)(duty_cycle * ICR1);
}

/* Set PWM duty cycle for channel B */
void set_pwm_duty_cycle_B(float duty_cycle) {
  duty_cycle = constrain(duty_cycle, 0.0, 1.0);
  OCR1B = (int)(duty_cycle * ICR1);
}
