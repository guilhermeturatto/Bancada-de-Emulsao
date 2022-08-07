#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#include "hall.h"
#include "modbus_RS485.h"
#include "DAC_MCP4725.h"

volatile uint8_t mode = 0;
volatile uint8_t dacmode = 100;
volatile uint16_t velREF = -1280;

/*
void UARTIntHandler(void)
{
    uint32_t ui32Status;
    char ReadChar;

    ui32Status = UARTIntStatus(UART0_BASE, true); //get interrupt status

    UARTIntClear(UART0_BASE, ui32Status); //clear the asserted interrupts

    while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
    {
        ReadChar = UARTCharGetNonBlocking(UART0_BASE);
        UARTCharPutNonBlocking(UART0_BASE, RefVelocity_msg[i]); //echo character
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2); //blink LED
        SysCtlDelay(SysCtlClockGet() / (1000 * 3)); //delay ~1 msec
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0); //turn off LED

        i = i==1 ? 0:1;
    }
}
*/

int main(void)
{

   //Configura clock para 40MHz
   SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

   //Configura comunicacao USB com o computador
      SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); //Habilita periferico UART0 para comunicacao
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); //Habilita periferico GPIOA onde estao os pinos do UART0

      GPIOPinConfigure(GPIO_PA0_U0RX); //Configura PA0 como RX da UART0
      GPIOPinConfigure(GPIO_PA1_U0TX); //Configura PA1 como TX da UART0
      GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); //Configura os pinos GPIO como RX e TX da UART0

      UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
          (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));


    //Habilita UART para comunicacao com o modulo XYK485
    ModbusConfigUART();

    //Habilita I2C para comunicacao com o DAC
    InitI2C0();

    char input;

    //Habilita GPIO e pinos dos LEDS
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); //enable GPIO port for LED
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3); //enable pin for LED PF2

    while(1)
    {
        UARTCharPut(UART0_BASE, 'H');
        switch(mode)
        {
        case 1:
            UARTCharPut(UART0_BASE, 'L');

            //LIGA MOTOR NA VELOCIDADE NOMINAL
            ModbusSend(ADDR_INVERSOR_1, WRITE, P683, velREF);
            ModbusSend(ADDR_INVERSOR_1, WRITE, P682,  MOTOR_ON);

            mode = 0;
            continue;

        case 2:
            UARTCharPut(UART0_BASE, 'D');

            //DESLIGA MOTOR
            ModbusSend(ADDR_INVERSOR_1, WRITE, P682, MOTOR_OFF);

            mode = 0;
            continue;

        case 3:
            //INCREMENTA VEL
            UARTCharPut(UART0_BASE, '+');

            velREF += 50;
            ModbusSend(ADDR_INVERSOR_1, WRITE, P683, velREF);
            ModbusSend(ADDR_INVERSOR_1, WRITE, P682,  MOTOR_ON);

            mode = 0;
            continue;

        case 4:
            //DECREMENTA VEL
           UARTCharPut(UART0_BASE, '-');

           velREF -= 50;
           ModbusSend(ADDR_INVERSOR_1, WRITE, P683, velREF);

           mode = 0;
           continue;

        //Teste DAC
        case 5:
            UARTCharPut(UART0_BASE, 'V');
            while(1){
                switch(dacmode)
                {
                case 0:
                    UARTCharPut(UART0_BASE, '0');
                    MCP4725Send((uint16_t)(0));
                    dacmode = 100;
                    continue;
                case 1:
                    UARTCharPut(UART0_BASE, '1');
                    MCP4725Send((uint16_t)(4095*1/5));
                    dacmode = 100;
                    continue;
                case 2:
                    UARTCharPut(UART0_BASE, '2');
                    MCP4725Send((uint16_t)(4095*2/5));
                    dacmode = 100;
                    continue;
                case 3:
                    UARTCharPut(UART0_BASE, '3');
                    MCP4725Send((uint16_t)(4095*3/5));
                    dacmode = 100;
                    continue;
                case 4:
                    UARTCharPut(UART0_BASE, '4');
                    MCP4725Send((uint16_t)(4095*4/5));
                    dacmode = 100;
                    continue;
                case 5:
                    UARTCharPut(UART0_BASE, '5');
                    MCP4725Send((uint16_t)(4095*5/5));
                    dacmode = 100;
                    continue;

                case 10:
                    dacmode = 100;
                    break;

                default:
                    input = UARTCharGet(UART0_BASE);

                    if(input == '0')
                        dacmode = 0;
                    else if(input == '1')
                        dacmode = 1;
                    else if(input == '2')
                        dacmode = 2;
                    else if(input == '3')
                        dacmode = 3;
                    else if(input == '4')
                        dacmode = 4;
                    else if(input == '5')
                        dacmode = 5;
                    else
                        dacmode = 10;

                    continue;
                }
                break;
            }

            mode = 0;
            continue;

        default:
            input = UARTCharGet(UART0_BASE);

            if(input == 'L')
                mode = 1;
            else if(input == 'D')
                mode = 2;
            else if(input == '+')
                mode = 3;
            else if(input == '-')
                mode = 4;
            else if(input == 'V')
                mode = 5;
        }
    }
}
