#include "tim.h"
#include "serial.h"
void set_motor_pwm(uint16_t*datapwm);
extern uint16_t pwm[10];
void set_motor_pwm(uint16_t*datapwm);
void pwm_init();
void pwm_stop();