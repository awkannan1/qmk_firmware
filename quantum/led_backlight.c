#if defined(BACKLIGHT_ENABLE) && defined(BACKLIGHT_PIN)

static const uint8_t backlight_pin = BACKLIGHT_PIN;

// depending on the pin, we use a different output compare unit
#if BACKLIGHT_PIN == B7
#  define TCCRxA TCCR1A
#  define TCCRxB TCCR1B
#  define COMxx1 COM1C1
#  define OCRxx  OCR1C
#  define ICRx   ICR1
#elif BACKLIGHT_PIN == B6
#  define TCCRxA TCCR1A
#  define TCCRxB TCCR1B
#  define COMxx1 COM1B1
#  define OCRxx  OCR1B
#  define ICRx   ICR1
#elif BACKLIGHT_PIN == B5
#  define TCCRxA TCCR1A
#  define TCCRxB TCCR1B
#  define COMxx1 COM1A1
#  define OCRxx  OCR1A
#  define ICRx   ICR1
#elif BACKLIGHT_PIN == C6
#  define TCCRxA TCCR3A
#  define TCCRxB TCCR3B
#  define COMxx1 COM1A1
#  define OCRxx  OCR3A
#  define ICRx   ICR3
#else
#  define NO_HARDWARE_PWM
#endif

#ifndef BACKLIGHT_ON_STATE
#define BACKLIGHT_ON_STATE 0
#endif

#ifdef NO_HARDWARE_PWM // pwm through software

__attribute__ ((weak))
void backlight_init_ports(void)
{
  // Setup backlight pin as output and output to on state.
  // DDRx |= n
  _SFR_IO8((backlight_pin >> 4) + 1) |= _BV(backlight_pin & 0xF);
  #if BACKLIGHT_ON_STATE == 0
    // PORTx &= ~n
    _SFR_IO8((backlight_pin >> 4) + 2) &= ~_BV(backlight_pin & 0xF);
  #else
    // PORTx |= n
    _SFR_IO8((backlight_pin >> 4) + 2) |= _BV(backlight_pin & 0xF);
  #endif
}

__attribute__ ((weak))
void backlight_set(uint8_t level) {}

uint8_t backlight_tick = 0;

#ifndef BACKLIGHT_CUSTOM_DRIVER
void backlight_task(void) {
  if ((0xFFFF >> ((BACKLIGHT_LEVELS - get_backlight_level()) * ((BACKLIGHT_LEVELS + 1) / 2))) & (1 << backlight_tick)) {
    #if BACKLIGHT_ON_STATE == 0
      // PORTx &= ~n
      _SFR_IO8((backlight_pin >> 4) + 2) &= ~_BV(backlight_pin & 0xF);
    #else
      // PORTx |= n
      _SFR_IO8((backlight_pin >> 4) + 2) |= _BV(backlight_pin & 0xF);
    #endif
  } else {
    #if BACKLIGHT_ON_STATE == 0
      // PORTx |= n
      _SFR_IO8((backlight_pin >> 4) + 2) |= _BV(backlight_pin & 0xF);
    #else
      // PORTx &= ~n
      _SFR_IO8((backlight_pin >> 4) + 2) &= ~_BV(backlight_pin & 0xF);
    #endif
  }
  backlight_tick = (backlight_tick + 1) % 16;
}
#endif

#ifdef BACKLIGHT_BREATHING
  #ifndef BACKLIGHT_CUSTOM_DRIVER
  #error "Backlight breathing only available with hardware PWM. Please disable."
  #endif
#endif

#else // pwm through timer

#define TIMER_TOP 0xFFFFU

// See http://jared.geek.nz/2013/feb/linear-led-pwm
static uint16_t cie_lightness(uint16_t v) {
  if (v <= 5243) // if below 8% of max
    return v / 9; // same as dividing by 900%
  else {
    uint32_t y = (((uint32_t) v + 10486) << 8) / (10486 + 0xFFFFUL); // add 16% of max and compare
    // to get a useful result with integer division, we shift left in the expression above
    // and revert what we've done again after squaring.
    y = y * y * y >> 8;
    if (y > 0xFFFFUL) // prevent overflow
      return 0xFFFFU;
    else
      return (uint16_t) y;
  }
}

// range for val is [0..TIMER_TOP]. PWM pin is high while the timer count is below val.
static inline void set_pwm(uint16_t val) {
	OCRxx = val;
}

#ifndef BACKLIGHT_CUSTOM_DRIVER
__attribute__ ((weak))
void backlight_set(uint8_t level) {
  if (level > BACKLIGHT_LEVELS)
    level = BACKLIGHT_LEVELS;

  if (level == 0) {
    // Turn off PWM control on backlight pin
    TCCRxA &= ~(_BV(COMxx1));
  } else {
    // Turn on PWM control of backlight pin
    TCCRxA |= _BV(COMxx1);
  }
  // Set the brightness
  set_pwm(cie_lightness(TIMER_TOP * (uint32_t)level / BACKLIGHT_LEVELS));
}

void backlight_task(void) {}
#endif  // BACKLIGHT_CUSTOM_DRIVER

#ifdef BACKLIGHT_BREATHING

#define BREATHING_NO_HALT  0
#define BREATHING_HALT_OFF 1
#define BREATHING_HALT_ON  2
#define BREATHING_STEPS 128

static uint8_t breathing_period = BREATHING_PERIOD;
static uint8_t breathing_halt = BREATHING_NO_HALT;
static uint16_t breathing_counter = 0;

bool is_breathing(void) {
    return !!(TIMSK1 & _BV(TOIE1));
}

#define breathing_interrupt_enable() do {TIMSK1 |= _BV(TOIE1);} while (0)
#define breathing_interrupt_disable() do {TIMSK1 &= ~_BV(TOIE1);} while (0)
#define breathing_min() do {breathing_counter = 0;} while (0)
#define breathing_max() do {breathing_counter = breathing_period * 244 / 2;} while (0)

void breathing_enable(void)
{
  breathing_counter = 0;
  breathing_halt = BREATHING_NO_HALT;
  breathing_interrupt_enable();
}

void breathing_pulse(void)
{
    if (get_backlight_level() == 0)
      breathing_min();
    else
      breathing_max();
    breathing_halt = BREATHING_HALT_ON;
    breathing_interrupt_enable();
}

void breathing_disable(void)
{
    breathing_interrupt_disable();
    // Restore backlight level
    backlight_set(get_backlight_level());
}

void breathing_self_disable(void)
{
  if (get_backlight_level() == 0)
    breathing_halt = BREATHING_HALT_OFF;
  else
    breathing_halt = BREATHING_HALT_ON;
}

void breathing_toggle(void) {
  if (is_breathing())
    breathing_disable();
  else
    breathing_enable();
}

void breathing_period_set(uint8_t value)
{
  if (!value)
    value = 1;
  breathing_period = value;
}

void breathing_period_default(void) {
  breathing_period_set(BREATHING_PERIOD);
}

void breathing_period_inc(void)
{
  breathing_period_set(breathing_period+1);
}

void breathing_period_dec(void)
{
  breathing_period_set(breathing_period-1);
}

/* To generate breathing curve in python:
 * from math import sin, pi; [int(sin(x/128.0*pi)**4*255) for x in range(128)]
 */
static const uint8_t breathing_table[BREATHING_STEPS] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 17, 20, 24, 28, 32, 36, 41, 46, 51, 57, 63, 70, 76, 83, 91, 98, 106, 113, 121, 129, 138, 146, 154, 162, 170, 178, 185, 193, 200, 207, 213, 220, 225, 231, 235, 240, 244, 247, 250, 252, 253, 254, 255, 254, 253, 252, 250, 247, 244, 240, 235, 231, 225, 220, 213, 207, 200, 193, 185, 178, 170, 162, 154, 146, 138, 129, 121, 113, 106, 98, 91, 83, 76, 70, 63, 57, 51, 46, 41, 36, 32, 28, 24, 20, 17, 15, 12, 10, 8, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Use this before the cie_lightness function.
static inline uint16_t scale_backlight(uint16_t v) {
  return v / BACKLIGHT_LEVELS * get_backlight_level();
}

/* Assuming a 16MHz CPU clock and a timer that resets at 64k (ICR1), the following interrupt handler will run
 * about 244 times per second.
 */
ISR(TIMER1_OVF_vect)
{
  uint16_t interval = (uint16_t) breathing_period * 244 / BREATHING_STEPS;
  // resetting after one period to prevent ugly reset at overflow.
  breathing_counter = (breathing_counter + 1) % (breathing_period * 244);
  uint8_t index = breathing_counter / interval % BREATHING_STEPS;

  if (((breathing_halt == BREATHING_HALT_ON) && (index == BREATHING_STEPS / 2)) ||
      ((breathing_halt == BREATHING_HALT_OFF) && (index == BREATHING_STEPS - 1)))
  {
      breathing_interrupt_disable();
  }

  set_pwm(cie_lightness(scale_backlight((uint16_t) pgm_read_byte(&breathing_table[index]) * 0x0101U)));
}

#endif // BACKLIGHT_BREATHING

__attribute__ ((weak))
void backlight_init_ports(void)
{
  // Setup backlight pin as output and output to on state.
  // DDRx |= n
  _SFR_IO8((backlight_pin >> 4) + 1) |= _BV(backlight_pin & 0xF);
  #if BACKLIGHT_ON_STATE == 0
    // PORTx &= ~n
    _SFR_IO8((backlight_pin >> 4) + 2) &= ~_BV(backlight_pin & 0xF);
  #else
    // PORTx |= n
    _SFR_IO8((backlight_pin >> 4) + 2) |= _BV(backlight_pin & 0xF);
  #endif
  // I could write a wall of text here to explain... but TL;DW
  // Go read the ATmega32u4 datasheet.
  // And this: http://blog.saikoled.com/post/43165849837/secret-konami-cheat-code-to-high-resolution-pwm-on

  // Pin PB7 = OCR1C (Timer 1, Channel C)
  // Compare Output Mode = Clear on compare match, Channel C = COM1C1=1 COM1C0=0
  // (i.e. start high, go low when counter matches.)
  // WGM Mode 14 (Fast PWM) = WGM13=1 WGM12=1 WGM11=1 WGM10=0
  // Clock Select = clk/1 (no prescaling) = CS12=0 CS11=0 CS10=1

  /*
  14.8.3:
  "In fast PWM mode, the compare units allow generation of PWM waveforms on the OCnx pins. Setting the COMnx1:0 bits to two will produce a non-inverted PWM [..]."
  "In fast PWM mode the counter is incremented until the counter value matches either one of the fixed values 0x00FF, 0x01FF, or 0x03FF (WGMn3:0 = 5, 6, or 7), the value in ICRn (WGMn3:0 = 14), or the value in OCRnA (WGMn3:0 = 15)."
  */
  TCCRxA = _BV(COMxx1) | _BV(WGM11); // = 0b00001010;
  TCCRxB = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // = 0b00011001;
  // Use full 16-bit resolution. Counter counts to ICR1 before reset to 0.
  ICRx = TIMER_TOP;

  backlight_init();
  #ifdef BACKLIGHT_BREATHING
    breathing_enable();
  #endif
}

#endif // NO_HARDWARE_PWM

#else // backlight

__attribute__ ((weak))
void backlight_init_ports(void) {}

__attribute__ ((weak))
void backlight_set(uint8_t level) {}

#endif // backlight
