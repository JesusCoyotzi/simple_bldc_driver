#include <msp430.h> 
#include <math.h>


#define DIREE BIT0 //Dirección para la EEPROM
//Bits para conteo de posición
#define EDGE BIT3	//Detecta flanco
#define DIR BIT4	//Detecta dirección del flanco

#define UP BIT3		//Cuenta Arriba
#define DWN BIT4	//Cuenta Abajo

volatile long mPos; //Posición Medida
long dPos=0;		//Posición Deseada
volatile float PID, e,e_p,e_int=0; 	//Variables de control

float const kp=25, ki=1; //Constantes del controlador
float const ts=0.00052;

//Lectura del ADC
void analogRead(void)	
{
	ADC10CTL1=INCH_5; 					//Seleccionar canal
	ADC10CTL0 |= ENC|ADC10SC;			//Encender ADC	
	while (ADC10CTL1 & ADC10BUSY);		//Esperar a terminar la medición
	ADC10CTL0 &= ~ENC;					//Apagar ADC
}

void ctrlLaw()
{
			dPos=ADC10MEM; //Leer registro del ADC para referencia
	    	e=dPos-mPos; //Error
	    	e_int=e_int+ts*(e); //Integral discreta

	    	if(e<=12 && e>=-12)
	    	{
	    		TA1CCR1=0;
	    	}
	    	else
	    	{

	    		PID=e*kp+ki*e_int; //Ley de control

	    		if(e>0)
	    		{
	    			P1OUT &= ~DIREE;  //Dirección
	    			TA1CCR1=abs(PID); //Valor

	    		}
	    		else
	    		{
	    			P1OUT |= DIREE;
	    			TA1CCR1=abs(PID);
	    		}
	    	}
	    	analogRead();

}

int main(void) {

	WDTCTL = WDTPW | WDTHOLD;	// Detener el perro de guardia

    BCSCTL1= CALBC1_16MHZ;		//Reloj a 16 megahertz 
    DCOCTL=CALDCO_16MHZ;

    //Configuración pines de entrada y salida
    P1SEL=0x00; 
    P1DIR |=DIREE; 
    P1REN |=DIREE;
    P1OUT &= ~DIREE;

    //Configuración de pines de interrupción
    P1IE |=UP;
    P1IE |=DWN;

    P1IFG &= ~DWN;
    P1IFG &= ~UP;


    //Configuración del Timer para Ley de control timer 0
    TA0CCTL0=CCIE;
    TA0CTL=TASSEL_2+MC_1+ID_2;  //SubmasterClock, divisor de 4
    TA0CCR0=2048;				//1953.13 [hz]

    P1DIR |= BIT6;
    P1OUT &= ~BIT6;


    //PWM setup
    //Configuración para el pin de PWM, Timer 1
    P2DIR |=BIT1;
    P2SEL |= BIT1;
    TA1CTL |= TASSEL_2 + MC_1+ ID_1; //SMCLK  divisor 2
    TA1CCR0 |= 1024+1; 				 //7804.88 [hz]
    TA1CCTL1 |= OUTMOD_6;
    TA1CCR1 |= 0;
   
    //Inicializar ADC
    ADC10CTL0=ADC10SHT_2+ADC10ON; //Seleccionar fuente del circuito de muestreo y encender el ADC
    ADC10AE0 |= BIT5; // Seleccionar el pin 5 para conexión

    //Habilitar interrupciones por hardware
    _BIS_SR(GIE); //Statur reg General Interrupt Enable
    
    //Primera Lectura del ADC
    analogRead();
    dPos=ADC10MEM;

    while(1)
    {

    	//La ley de control se ejecuta en la interrupción del timer
    	//EL conteo en la interrupción del puerto
    }
    

}


//ISR para conteo de posición
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
	//Detecta que pin levanto la interrupción y aumenta o decrementa el contador
	if(P1IFG & UP)
	{
		mPos++;
		P1IFG&=~UP;
	}
	else if(P1IFG & DWN)
	{
		mPos--;
		P1IFG&=~DWN;
	}

	//MEtodo de conteo alternativo
	/*if(P1IN & BIT4)
	{
		mPos++;
	}
	else
	{
		mPos--;
	}

	P1IFG&=~EDGE;
	*/

}


//Interrupción del timer
#pragma vector= TIMER0_A0_VECTOR
__interrupt void timerInt(void)
{
	ctrlLaw(); //Ley de control
	TA0CCTL0 &= ~TAIFG;
}

