#include "Compiler.h"
#include "HardwareProfile.h"
#include "uart1.h"

//----------------------------------------------------------------------
//LED点滅:
//		(1<<ncnt) 回に一回の割合で、LED1 を反転させる.
//
//		この関数はメインループのどこかに呼び出しを入れるだけ.
//
//----------------------------------------------------------------------
//
void led_blink(int ncnt)
{
	int cnt = (1<<ncnt);
    static int led_count=0;
    led_count++;
    if(	led_count > cnt) {
    	led_count = 0;
		mLED_1_Toggle();
	}
}

//----------------------------------------------------------------------
//LED点滅テスト:
//	適当なインターバルでLED点滅を繰り返す.
//	この関数は無限ループ.
//----------------------------------------------------------------------
//
void led_test(void)
{
	mInitAllLEDs();
	while(1) {
		led_blink(19);
	}
}

void _T4Interrupt( void );

void __attribute__((interrupt,nomips16,noinline)) _Tmr4Interrupt()
{
//	_T4Interrupt();
}

//void	Wait(unsigned int B){}
