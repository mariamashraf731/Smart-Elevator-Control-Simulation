#include "C8051F020.h"

// --- Global Variables ---
int floor = 0;              // Current floor
unsigned int up_request[5]   = {0, 0, 0, 0, 0};
unsigned int floor_request[5]= {0, 0, 0, 0, 0};
unsigned int down_request[5] = {0, 0, 0, 0, 0};

// --- Sensors & Actuators Mapping ---
sbit redled     = P2^3;     // Door Closed / Moving
sbit greenled   = P0^7;     // Door Open / Idle
sbit OpenDoor   = P2^7;     // Manual Open Button
sbit overload   = P1^7;     // Overload Indicator

// Motor H-Bridge Control
sbit motor1     = P2^0;     
sbit motor2     = P2^1;     

// IR Sensors for Passenger Counting
sbit ir1        = P2^2;     
sbit ir2        = P2^5;     

// Input Buttons (Internal & External)
sbit floor0     = P0^2;
sbit floor1     = P0^3;
sbit floor2     = P0^4;
sbit floor3     = P0^5;
sbit floor4     = P0^6;
sbit floor0_down= P2^4;
sbit floor2_down= P2^6;

// State Variables
int i = 0;
int doorstate = 0;
int enter = 0;
int exit = 0;
int persons = 0; 

// --- Functions ---

void timer0_init(void) {
    CKCON &= 0xF0;          // Timer 0 uses SysClock/12 
    TMOD = (TMOD & 0xF0) | 0x01; // Mode 1: 16-bit timer
    TH0 = 0xFF;             // Reload value for ~240ms logic
    TL0 = 0x63; 
    ET0 = 1;                // Enable Timer0 Interrupt
}

// Blocking Delays (Simulation purpose)
void motor_delay() {
    int i, j;
    for (i = 0; i < 1000; ++i)
        for (j = 0; j < 100; ++j);
}

void motor_delay1() {
    int i, j;
    for (i = 0; i < 1000; ++i)
        for (j = 0; j < 90; ++j);
}

void delay() {
    int i, j;
    for (i = 0; i < 500; ++i)
        for (j = 0; j < 100; ++j);
}

// Passenger Counting Logic
void check_sensors() {
    // Logic to detect entry/exit sequence using 2 IR sensors
    if ((ir1 == 0) && (ir2 == 1)) { 
        if (exit == 0) enter = 1;
        if (exit == 1) { persons--; exit = 0; delay(); }
    }
    if ((ir1 == 1) && (ir2 == 0)) { 
        if (enter == 0) exit = 1;
        if (enter == 1) { persons++; enter = 0; delay(); }
    }
}

void door_delay_monitoring() {
    int i, j;
    for (i = 0; i < 1000; ++i) {
        for (j = 0; j < 1000; ++j) {
            check_sensors(); // Keep monitoring while door is open
        }
    }
}

void open_door() {
    redled = 1;
    greenled = 0;
    door_delay_monitoring();
    
    // Safety Check: Overload
    if (persons > 4) {
        overload = 1;
        open_door(); // Keep door open if overloaded
    } else {
        overload = 0;
    }
    
    redled = 0;
    greenled = 1;
}

void up() {
    motor1 = 1; motor2 = 0;
    motor_delay();
    motor1 = 0; motor2 = 1; // Braking / Phase shift simulation
    motor_delay1();
    motor2 = 0;
}

void down() {
    motor1 = 0; motor2 = 1;
    motor_delay();
    motor2 = 0; motor1 = 1;
    motor_delay1();
    motor1 = 0;
}

void requests(void) {
    // Poll all buttons and update request arrays
    if (!floor0) floor_request[0] = 1;
    if (!floor1) floor_request[1] = 1;
    if (!floor2) floor_request[2] = 1;
    if (!floor3) floor_request[3] = 1;
    if (!floor4) floor_request[4] = 1;

    // External Requests
    if (!floor0_down) down_request[0] = 1;
    if (!floor2_down) down_request[2] = 1;
    
    // Port 5 Polling for Up requests
    if (!(P5 & 0x01)) up_request[1] = 1;
    if (!(P5 & 0x02)) up_request[2] = 1;
    if (!(P5 & 0x04)) up_request[3] = 1;
    if (!(P5 & 0x08)) up_request[4] = 1;

    if ((!OpenDoor && !doorstate)) {
        open_door();
    }
}

// ISR: Periodic polling for requests
void timer0_ISR() interrupt 1 {
    TF0 = 0;
    requests();
}

void seg_display(int fl) {
    if (fl == 0) P1 = 0x3F; // '0'
    else if (fl == 1) P1 = 0x06; // '1'
    else if (fl == 2) P1 = 0x5B; // '2'
    else if (fl == 3) P1 = 0x4F; // '3'
    else if (fl == 4) P1 = 0x66; // '4'
}

void main(void) {
    // System Init
    WDTCN = 0xDE; WDTCN = 0xAD; // Disable Watchdog
    OSCICN = 0x14;              // 2MHz internal clock
    XBR0 = 0x00; XBR1 = 0x00; XBR2 = 0x40; // Crossbar enabled
    
    // Port Configuration
    P0MDOUT |= 0x80;
    P1MDOUT |= 0x7F;
    P2MDOUT |= 0x00;

    timer0_init();
    EA = 1; TR0 = 1; // Enable Global Interrupts & Timer0

    overload = 0;
    redled = 0; greenled = 1;
    requests();

    while (1) {
        seg_display(floor);
        
        // UP Direction Sweep (SCAN Algorithm)
        for (i = 0; i < 5; i++) {
            if ((floor_request[i] || up_request[i] || down_request[i]) && i > floor) {
                while (floor != i) {
                    doorstate = 1;
                    up();
                    floor++;
                    seg_display(floor);
                    
                    // Check for intermediate stops
                    if (floor_request[floor] || up_request[floor] || down_request[floor]) {
                        floor_request[floor] = 0;
                        up_request[floor] = 0;
                        down_request[floor] = 0;
                        doorstate = 0;
                        open_door();
                    }
                }
            }
        }

        // DOWN Direction Sweep
        for (i = 4; i >= 0; i--) {
            if ((floor_request[i] || down_request[i] || up_request[i]) && i < floor) {
                while (floor != i) {
                    doorstate = 1;
                    down();
                    floor--;
                    seg_display(floor);
                    
                    if (floor_request[floor] || down_request[floor] || up_request[floor]) {
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