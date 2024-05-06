/**/
/* Allows the interface between a control interface and the engage motor on EcoMaua vehicle */
/**/
/* Use Tools >> Board >> Digistump AVR Board >> Digispark (Default - 16.5mhz) */
/**/

#define CTRL_IN_ENGAGE_PIN 0  /* Pin 5 on ATtiny85 */
#define PWM_OC1A_PIN 1        /* Pin 6 on ATtiny85, OC1A */
#define CTRL_IN_GNDREF_PIN 2  /* Pin 7 on ATtiny85 */
#define CTRL_OUT_ENABLE_PIN 3 /* Pin 2 on ATtiny85 */
#define PWM_OC1B_PIN 4        /* Pin 3 on ATtiny85, OC1B */

void inline timer1_reset_timer() {
  /* Reset Timer1 */
  TCCR1 = 0;
}

void inline timer1_stop_timer() {
  /* Stop Timer1 */
  uint8_t timer1_TCCR1_register = TCCR1;
  timer1_TCCR1_register &= ~(_BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10));
  TCCR1 = timer1_TCCR1_register;
}

void inline timer1_start_timer_prescale_CK4() {
  /* Start Timer1 with CK/4 prescaler */
  uint8_t timer1_TCCR1_register = TCCR1;
  timer1_TCCR1_register &= ~(_BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10));
  timer1_TCCR1_register |= _BV(CS11) | _BV(CS10);
  TCCR1 = timer1_TCCR1_register;
}

void inline timer1_config_pwm_20000_hz() {
  /* Set counters to 20000 Hz with prescale CK/4 */
  OCR1C = 206;  /* Set the TOP value for 20kHz (F_CPU / 20000 / Prescaler - 1) */
  OCR1A = 103;  /* Set Compare Match value for 50% duty cycle (OCR1C / 2) */
  OCR1B = 103;  /* Set Compare Match value for 50% duty cycle (OCR1C / 2) */
}

void inline timer1_config_pwm_output_OC1A() {
  /* Enable PWM A generator and set the OC1A as set on compare match */
  uint8_t timer1_TCCR1_register = TCCR1;
  uint8_t timer1_GTCCR_register = GTCCR;
  timer1_TCCR1_register |= _BV(PWM1A) | _BV(COM1A1) | _BV(COM1A0);
  timer1_GTCCR_register &= ~(_BV(FOC1A));
  TCCR1 = timer1_TCCR1_register;
  GTCCR = timer1_GTCCR_register;
}

void inline timer1_config_pwm_output_OC1B() {
  /* Enable PWM B generator and set the OC1B as set on compare match */
  uint8_t timer1_GTCCR_register = GTCCR;
  timer1_GTCCR_register &= ~(_BV(FOC1B));
  timer1_GTCCR_register |= _BV(PWM1B) | _BV(COM1B1) | _BV(COM1B0);
  GTCCR = timer1_GTCCR_register;
}

void inline timer1_force_output_OC1A(bool output_value) {
  /* Force OC1A output to a value (disable PWM) */
  uint8_t timer1_TCCR1_register = TCCR1;
  uint8_t timer1_GTCCR_register = GTCCR;
  timer1_TCCR1_register &= ~(_BV(PWM1A) | _BV(COM1A1) | _BV(COM1A0));
  if (output_value) {
    timer1_TCCR1_register |= _BV(COM1A1) | _BV(COM1A0);
  } else {
    timer1_TCCR1_register |= _BV(COM1A1);
  }
  timer1_GTCCR_register |= _BV(FOC1A);
  GTCCR = timer1_GTCCR_register;
  TCCR1 = timer1_TCCR1_register;
}

void inline timer1_force_output_OC1B(bool output_value) {
  /* Force OC1B output to a value (disable PWM) */
  uint8_t timer1_GTCCR_register = GTCCR;
  timer1_GTCCR_register &= ~(_BV(PWM1B) | _BV(COM1B1) | _BV(COM1B0));
  if (output_value) {
    timer1_GTCCR_register |= _BV(COM1B1) | _BV(COM1B0);
  } else {
    timer1_GTCCR_register |= _BV(COM1B1);
  }
  timer1_GTCCR_register |= _BV(FOC1B);
  GTCCR = timer1_GTCCR_register;
}

void inline adjusted_delay(uint64_t delay_ms) {
  delay((uint64_t)(delay_ms * 37.037037037037037037037037037037f));
}

void inline engage_procedure(bool engage_flag) {
    if (engage_flag) {
      /* Engage motor activated */
      timer1_force_output_OC1B(false);
      timer1_config_pwm_output_OC1A();
    } else {
      /* Engage motor deactivated */
      timer1_force_output_OC1A(false);
      timer1_config_pwm_output_OC1B();
    }
}

void setup() {

  /* Set the CTRL_IN pins as inputs */
  // pinMode(CTRL_IN_ENABLE_PIN, INPUT_PULLUP);
  pinMode(CTRL_IN_ENGAGE_PIN, INPUT_PULLUP);
  pinMode(CTRL_IN_GNDREF_PIN, OUTPUT);
  digitalWrite(CTRL_IN_GNDREF_PIN, LOW);

  /* Set the CTRL_OUT pins as output */
  pinMode(CTRL_OUT_ENABLE_PIN, OUTPUT);
  digitalWrite(CTRL_OUT_ENABLE_PIN, LOW);

  /* Set the PWM pins as outputs */
  pinMode(PWM_OC1A_PIN, OUTPUT);
  pinMode(PWM_OC1B_PIN, OUTPUT);

  // /* Configure Timer1 to output PWM */
  timer1_reset_timer();
  timer1_stop_timer();

  timer1_config_pwm_20000_hz();

  timer1_force_output_OC1B(false);
  timer1_force_output_OC1B(false);

}

void loop() {

  /* Check enable pin */
  // if (digitalRead(CTRL_IN_ENABLE_PIN)) {
  if (1) {
    /* Engage motor enabled */
    digitalWrite(CTRL_OUT_ENABLE_PIN, HIGH);
    timer1_start_timer_prescale_CK4();
    engage_procedure((bool)digitalRead(CTRL_IN_ENGAGE_PIN));
  } else {
    /* Engage motor disabled */
    digitalWrite(CTRL_OUT_ENABLE_PIN, LOW);
    timer1_force_output_OC1A(false);
    timer1_force_output_OC1B(false);
    timer1_stop_timer();
  }

  adjusted_delay(1);

}
