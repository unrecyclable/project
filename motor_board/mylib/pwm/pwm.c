#include "pwm.h"
#include "main.h"

#define netrual 1500
uint16_t pwm[10];

void pwm_init()
{
	MX_TIM1_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	for(int i=0;i<8;i++)
		pwm[i]=netrual;
	pwm[8]=1000;
	pwm[9]=1000;
	set_motor_pwm(pwm);
	//HAL_TIM_Base_Start(&htim2);
	//HAL_TIM_Base_Start(&htim3);
	//HAL_TIM_Base_Start(&htim4);
	
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4);
	
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_4);
	
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);
	//HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);


	
	
}
void pwm_stop()
{
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_4);
	HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_3);
	HAL_TIM_PWM_Stop(&htim2,TIM_CHANNEL_4);
	
	HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_1);
	//HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_2);
	
	
	
}

void set_motor_pwm(uint16_t*datapwm)
{
	


//		__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,datapwm[3]-20);	2
//	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,(2960-datapwm[2]));	4
//	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,(2960-datapwm[7]));	3
//	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_4,datapwm[5]-20);	1
//	
//	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1,(2960-datapwm[6])); 7	
//__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,datapwm[4]-20);	6
//	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_3,(2960-datapwm[0]));	8
//	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_4,datapwm[1]-20);	5
//	
//	__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_1,datapwm[8]);	
//	__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,datapwm[9]);	
	
	

			__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,datapwm[1]-20); 
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,(datapwm[3]-20));	
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,(2960-datapwm[2]));	
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_4,2960-datapwm[0]);	
	
	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1,(2960-datapwm[6]));	
__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_2,datapwm[5]-20);	
	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_3,(datapwm[7]-20));	
	__HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_4,2960-datapwm[4]);	
	
	__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_1,datapwm[8]);	
	//__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_2,datapwm[9]);	

}

	