#include "C8051F020.h"

/* -------------------------
   Global variables
------------------------- */
int floor = 0;                        // Current elevator floor

unsigned int up_request[5]   = {0,0,0,0,0};   // Up requests from floors
unsigned int floor_request[5]= {0,0,0,0,0};   // Requests from inside elevator
unsigned int down_request[5] = {0,0,0,0,0};   // Down requests from floors

int i = 0;

/* -------------------------
   LEDs and indicators
------------------------- */
sbit redled   = P2^3;     // Red LED (door closed / moving)
sbit greenled = P0^7;     // Green LED (door open)

/* -------------------------
   Door control
------------------------- */
int doorstate = 0;        // 0 = door open, 1 = door closed/moving
sbit OpenDoor = P2^7;     // Manual open door button

/* -------------------------
   Inside elevator buttons
------------------------- */
sbit floor0 = P0^2;
sbit floor1 = P0^3;
sbit floor2 = P0^4;
sbit floor3 = P0^5;
sbit floor4 = P0^6;

/* -------------------------
   IR sensors for people count
------------------------- */
sbit ir1 = P2^2;          // IR sensor 1
sbit ir2 = P2^5;          // IR sensor 2

int enter = 0;            // Person entering flag
int exit  = 0;            // Person exiting flag

/* -------------------------
   Overload detection
------------------------- */
sbit overload = P1^7;     // Overload indicator
int persons = 0;          // Number of people inside elevator

/* -------------------------
   Motor control
------------------------- */
sbit motor1 = P2^0;       // Motor direction control
sbit motor2 = P2^1;

sbit floor0_down = P2^4;  // Down button at floor 0
sbit floor2_down = P2^6;  // Down button at floor 2

/* -------------------------
   Timer0 initialization
------------------------- */
void timer0_init(void)
{
    CKCON &= 0xF0;                 // Timer0 uses SYSCLK/12
    TMOD = (TMOD & 0xF0) | 0x01;   // Timer0 Mode 1 (16-bit)
    TH0  = 0xFF;
    TL0  = 0xBF;                   // ~240 ms overflow
    ET0  = 1;                      // Enable Timer0 interrupt
}

/* -------------------------
   Motor movement delays
------------------------- */
void motor_delay()
{
    int i,j;
    for (i=0;i<1000;i++)
        for(j=0;j<100;j++);
}

void motor_delay1()
{
    int i,j;
    for (i=0;i<1000;i++)
        for(j=0;j<90;j++);
}

/* -------------------------
   Small delay (debounce)
------------------------- */
void delay()
{
    int i,j;
    for (i=0;i<500;i++)
        for(j=0;j<100;j++);
}

/* -------------------------
   Door delay + people counting
------------------------- */
void door_delay()
{
    int i,j;
    for (i=0;i<1000;i++)
    for (j=0;j<1000;j++)
    {
        // Person entering
        if ((ir1 == 0) && (ir2 == 1))
        {
            if(exit == 0) enter = 1;
            if(exit == 1){
                persons--;
                exit = 0;
                delay();
            }
        }

        // Person exiting
        if ((ir1 == 1) && (ir2 == 0))
        {
            if(enter == 0) exit = 1;
            if(enter == 1){
                persons++;
                enter = 0;
                delay();
            }
        }
    }
}

/* -------------------------
   Elevator movement
------------------------- */
void up()
{
    motor1 = 1; motor2 = 0;
    motor_delay();
    motor1 = 0; motor2 = 1;
    motor_delay1();
    motor2 = 0;
}

void down()
{
    motor1 = 0; motor2 = 1;
    motor_delay();
    motor2 = 0; motor1 = 1;
    motor_delay1();
    motor1 = 0;
}

/* -------------------------
   Door operation
------------------------- */
void open_door()
{
    redled = 1;
    greenled = 0;

    door_delay();

    // Overload protection
    if (persons > 4){
        overload = 1;
        open_door();   // keep door open
    }
    if (persons < 5)
        overload = 0;

    redled = 0;
    greenled = 1;
}

/* -------------------------
   Read all requests
------------------------- */
void requests(void)
{
    // Inside buttons
    if(!floor0) floor_request[0] = 1;
    if(!floor1) floor_request[1] = 1;
    if(!floor2) floor_request[2] = 1;
    if(!floor3) floor_request[3] = 1;
    if(!floor4) floor_request[4] = 1;

    // Down buttons
    if(!floor0_down) down_request[0] = 1;
    if(!floor2_down) down_request[2] = 1;

    // Up buttons on P5
    if(!(P5 & 0x01)) up_request[1] = 1;
    if(!(P5 & 0x02)) up_request[2] = 1;
    if(!(P5 & 0x04)) up_request[3] = 1;
    if(!(P5 & 0x08)) up_request[4] = 1;

    // Manual open door
    if(!OpenDoor && !doorstate)
        open_door();
}

/* -------------------------
   Timer0 ISR
------------------------- */
void timer0_ISR() interrupt 1
{
    TF0 = 0;
    requests();     // Periodically scan requests
}

/* -------------------------
   7-segment display
------------------------- */
void seg_dispaly(int floor)
{
    if(floor==0) P1 = 0x3F;
    else if(floor==1) P1 = 0x06;
    else if(floor==2) P1 = 0x5B;
    else if(floor==3) P1 = 0x4F;
    else if(floor==4) P1 = 0x66;
}

/* -------------------------
   MAIN
------------------------- */
void main(void)
{
    // Disable watchdog
    WDTCN = 0xDE;
    WDTCN = 0xAD;

    OSCICN = 0x14;       // 2 MHz internal clock

    // Crossbar config
    XBR0 = 0x00;
    XBR1 = 0x00;
    XBR2 = 0x40;

    // Port direction
    P0MDOUT |= 0x80;
    P1MDOUT |= 0x7F;
    P2MDOUT |= 0x00;

    timer0_init();
    EA  = 1;
    TR0 = 1;

    overload = 0;
    redled   = 0;
    greenled = 1;

    while(1)
    {
        seg_dispaly(floor);

        // Move UP
        for(i=0;i<5;i++)
        {
            if((floor_request[i] || up_request[i] || down_request[i]) && i > floor)
            {
                while(floor != i)
                {
                    doorstate = 1;
                    up();
                    floor++;
                    seg_dispaly(floor);

                    if(floor_request[floor] || up_request[floor] || down_request[floor])
                    {
                        floor_request[floor] = 0;
                        up_request[floor] = 0;
                        down_request[floor] = 0;
                        doorstate = 0;
                        open_door();
                    }
                }
            }
        }

        // Move DOWN
        for(i=4;i>=0;i--)
        {
            if((floor_request[i] || down_request[i] || up_request[i]) && i < floor)
            {
                while(floor != i)
                {
                    doorstate = 1;
                    down();
                    floor--;
                    seg_dispaly(floor);

                    if(floor_request[floor] || down_request[floor] || up_request[floor])
                    {
                        floor_request[floor] = 0;
                        down_request[floor] = 0;
                        up_request[floor] = 0;
                        doorstate = 0;
                        open_door();
                    }
                }
            }
        }
    }
}
