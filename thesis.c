#include <built_in.h>
#include <stdint.h>
#include <stdlib.h>

// LCD module connections
sbit LCD_RS at RD2_bit;
sbit LCD_EN at RD3_bit;
sbit LCD_D4 at RD4_bit;
sbit LCD_D5 at RD5_bit;
sbit LCD_D6 at RD6_bit;
sbit LCD_D7 at RD7_bit;

sbit LCD_RS_Direction at TRISD2_bit;
sbit LCD_EN_Direction at TRISD3_bit;
sbit LCD_D4_Direction at TRISD4_bit;
sbit LCD_D5_Direction at TRISD5_bit;
sbit LCD_D6_Direction at TRISD6_bit;
sbit LCD_D7_Direction at TRISD7_bit;
// End LCD module connections

// UART module connections
sbit UART1_Rx at RC7_bit;
sbit UART1_Tx at RC6_bit;

sbit UART1_Rx_Direction at TRISC7_bit;
sbit UART1_Tx_Direction at TRISC6_bit;
// End UART module connections

// Define trigger and echo pins for CONTAINERS  
#define TRIGGER_RW PORTB.F0
#define ECHO_RW    PORTB.F1
#define TRIGGER_DW PORTB.F2
#define ECHO_DW    PORTB.F3
#define TRIGGER_Res PORTB.F4
#define ECHO_Res   PORTB.F5

// Define trigger and echo pins for DOSERS
#define TRIGGER_UP PORTB.F6
#define ECHO_UP    PORTB.F7
#define TRIGGER_DWN PORTA.F0
#define ECHO_DWN    PORTA.F1
#define TRIGGER_Nutr PORTA.F2
#define ECHO_Nutr   PORTA.F3
#define pH_level_pin PORTA.F4

#define PB   PORTC.F0

// Valves
#define valve_RW PORTC.F1
#define valve_DW    PORTC.F2

// Peristaltic pumps
#define pH_UP_pump PORTC.F4
#define pH_DWN_pump    PORTC.F5
#define nutr_pump PORTD.F0

// Motor pump
#define motor_pump PORTD.F1

// Variables for CONTAINERS
int time_RW, distance_RW, distance_RWF;
int time_DW, distance_DW;
int time_Res, distance_Res;

// Define the variable to store the elapsed time
unsigned int time_counter = 0;

// Define the variable to store the number of seconds before UART transmission
const unsigned int UART_TRANSMISSION_INTERVAL = 60; // 60 seconds

// Variables for DOSERS
int time_UP, distance_UP;
int time_DWN, distance_DWN;
int time_Nutr, distance_Nutr;
int num;

float read_temperature_value, read_ph_value, pH_value;
double voltage_temperature,voltage_ph ;
int temperature_value, tds_value;

// Temporary variables for converting distances to strings
char txt[7], txt2[10];

void push_button() {
    if (PB == 0) {
        while (PB == 0);
        Lcd_Cmd(_LCD_CLEAR);
        num = (num < 3) ? num + 1 : 0;
    }
}

void trim(int temp_num){
        IntToStr(temp_num, txt);
        Ltrim(txt);
}

void display(int x){
   if (x==0){
        trim (distance_RW);
        Lcd_Out(1,1,"RW Level = ");
        Lcd_Out(1,12,txt);
        Lcd_Out(1,15,"cm");

        trim (distance_DW);
        Lcd_Out(2,1,"DW Level = ");
        Lcd_Out(2,12,txt);
        Lcd_Out(2,15,"cm");
   }else if (x == 1){
        trim(distance_Res);
        Lcd_Out(1,1,"RV Level: ");
        Lcd_Out(1,12,txt);
        Lcd_Out(1,15,"cm");

        trim(distance_UP);
        Lcd_Out(2,1,"pH UP   : ");
        Lcd_Out(2,12,txt);
        Lcd_Out(2,15,"cm");
   }else if(x == 2){
        trim(distance_DWN);
        Lcd_Out(1,1,"pH DWN  : ");
        Lcd_Out(1,12,txt);
        Lcd_Out(1,15,"cm");

        trim(distance_Nutr);
        Lcd_Out(2,1,"Nutrient: ");
        Lcd_Out(2,12,txt);
        Lcd_Out(2,15,"cm");
   } else {
        trim(temperature_value);
        lcd_out(1,1,"Thermal : ");
        Lcd_Out(1,12,txt);
        Lcd_Out(1,15,"*C");

        lcd_out(2,1,"PH Content:");
        FloattoStr(ph_value, txt2);
        Ltrim(txt2);
        Lcd_Out(2,12,txt2);
   }
}

// Open/close of containers
void controller() {
  // Making sure Res is always full
  if (distance_Res < 30){
    // Shifting happens here
    if (distance_RW > 50){
        valve_RW = 1;
        valve_DW = 0;
    } else{
         valve_RW = 0;
         if (distance_DW > 50){
             valve_DW = 1;
         } else{
             valve_DW = 0;
         }
    }
  } else{
     valve_RW = 0;
     valve_DW = 0;

     motor_pump = 0;

     // check ph sensor
     if (ph_value <= 5.5){
        pH_UP_pump = 1;
        pH_DWN_pump = 0;

     }else if (ph_value >= 6.5){
        pH_UP_pump = 0;
        pH_DWN_pump = 1;
     }else{
        pH_UP_pump = 0;
        pH_DWN_pump = 0;
     }


    //check tds sensor
    if(tds_value < 750){
        nutr_pump = 1;
    }else{
        nutr_pump = 0;
    }
  }


}

// Function to measure water level for RW sensor
void waterlevel_RW() {
    TRIGGER_RW = 0;           // Turn off the trigger pulse
    TRIGGER_RW = 1;           // Send a high pulse to the trigger pin
    Delay_us(10);             // Wait for 10 microseconds
    TRIGGER_RW = 0;           // Turn off the trigger pulse
    TMR1L = 0;                // Reset the Timer1 low byte
    TMR1H = 0;                // Reset the Timer1 high byte
    while (!ECHO_RW);         // Wait for the echo pin to go high
    T1CON.F0 = 1;             // Start Timer1

    while (ECHO_RW);          // Wait for the echo pin to go low
    T1CON.F0 = 0;             // Stop Timer1
    time_RW = (TMR1L | (TMR1H << 8));
    distance_RW = time_RW / 58.82;  // Convert time to distance (in centimeters)
    distance_RW = distance_RW + 12;

}

void waterlevel_DW() {
    TRIGGER_DW = 0;           // Turn off the trigger pulse
    TRIGGER_DW = 1;           // Send a high pulse to the trigger pin
    Delay_us(10);             // Wait for 10 microseconds
    TRIGGER_DW = 0;           // Turn off the trigger pulse
    TMR1L = 0;                // Reset the Timer1 low byte
    TMR1H = 0;                // Reset the Timer1 high byte
    while (!ECHO_DW);         // Wait for the echo pin to go high
    T1CON.F0 = 1;              // Start Timer1
    while (ECHO_DW);          // Wait for the echo pin to go low
    T1CON.F0 = 0;             // Stop Timer1
    time_DW = (TMR1L | (TMR1H << 8));
    distance_DW = time_DW / 58.82;  // Convert time to distance (in centimeters)
    distance_DW = distance_DW + 12;

}

void waterlevel_Res() {
    TRIGGER_Res = 0;           // Turn off the trigger pulse
    TRIGGER_Res = 1;           // Send a high pulse to the trigger pin
    Delay_us(10);             // Wait for 10 microseconds
    TRIGGER_Res = 0;           // Turn off the trigger pulse
    TMR1L = 0;                // Reset the Timer1 low byte
    TMR1H = 0;                // Reset the Timer1 high byte
    while (!ECHO_Res);         // Wait for the echo pin to go high
    T1CON.F0 = 1;             // Start Timer1
    while (ECHO_Res);          // Wait for the echo pin to go low
    T1CON.F0 = 0;             // Stop Timer1
    time_Res = (TMR1L | (TMR1H << 8));
    distance_Res = time_Res / 58.82;  // Convert time to distance (in centimeters)
    distance_Res = distance_Res + 12;
}

void waterlevel_UP() {
    TRIGGER_UP = 0;           // Turn off the trigger pulse
    TRIGGER_UP = 1;           // Send a high pulse to the trigger pin
    Delay_us(10);             // Wait for 10 microseconds
    TRIGGER_UP = 0;           // Turn off the trigger pulse
    TMR1L = 0;                // Reset the Timer1 low byte
    TMR1H = 0;                // Reset the Timer1 high byte
    while (!ECHO_UP);         // Wait for the echo pin to go high
    T1CON.F0 = 1;             // Start Timer1
    while (ECHO_UP);          // Wait for the echo pin to go low
    T1CON.F0 = 0;             // Stop Timer1
    time_UP = (TMR1L | (TMR1H << 8));
    distance_UP = time_UP / 58.82;  // Convert time to distance (in centimeters)
    distance_UP = distance_UP + 12;
}

void waterlevel_DWN() {
    TRIGGER_DWN = 0;           // Turn off the trigger pulse
    TRIGGER_DWN = 1;           // Send a high pulse to the trigger pin
    Delay_us(10);             // Wait for 10 microseconds
    TRIGGER_DWN = 0;           // Turn off the trigger pulse
    TMR1L = 0;                // Reset the Timer1 low byte
    TMR1H = 0;                // Reset the Timer1 high byte
    while (!ECHO_DWN);         // Wait for the echo pin to go high
    T1CON.F0 = 1;             // Start Timer1
    while (ECHO_DWN);          // Wait for the echo pin to go low
    T1CON.F0 = 0;             // Stop Timer1
    time_DWN = (TMR1L | (TMR1H << 8));
    distance_DWN = time_DWN / 58.82;  // Convert time to distance (in centimeters)
    distance_DWN = distance_DWN + 12;
}

void waterlevel_Nutr() {
    TRIGGER_Nutr = 0;           // Turn off the trigger pulse
    TRIGGER_Nutr = 1;           // Send a high pulse to the trigger pin
    Delay_us(10);             // Wait for 10 microseconds
    TRIGGER_Nutr = 0;           // Turn off the trigger pulse
    TMR1L = 0;                // Reset the Timer1 low byte
    TMR1H = 0;                // Reset the Timer1 high byte
    while (!ECHO_Nutr);         // Wait for the echo pin to go high
    T1CON.F0 = 1;             // Start Timer1
    while (ECHO_Nutr);          // Wait for the echo pin to go low
    T1CON.F0 = 0;             // Stop Timer1
    time_Nutr = (TMR1L | (TMR1H << 8));
    distance_Nutr = time_Nutr / 58.82;  // Convert time to distance (in centimeters)
    distance_Nutr = distance_Nutr + 12;
}

void temperature_reading() {
    read_temperature_value = ADC_Read(5);
    voltage_temperature = read_temperature_value * 0.0048828;
    temperature_value = voltage_temperature / 0.01;
}

void pH_reading() {
    read_ph_value = ADC_Read(6);
    voltage_ph = read_ph_value * 0.0048828;
    ph_value = 7+((2.5 - voltage_ph )/ 0.1841);
}

void tds_reading() {
//    read_ph_value = ADC_Read(7);
//    voltage_ph = read_ph_value * 0.0048828;
//    ph_value = 7+((2.5 - voltage_ph )/ 0.1841);
}

void uart_transmission(int a, int b, int c, int d, int e, int f, float g, float h){
    char a1[5], b1[5],c1[5], d1[5],e1[5], f1[5],g1[6], h1[6];  
    IntToStr(a, a1); 
    IntToStr(b, b1);
    IntToStr(c, c1);
    IntToStr(d, d1);
    IntToStr(e, e1); 
    IntToStr(f, f1);
    FloatToStr(g, g1);
    FloatToStr(h, h1);

    UART1_Write(Ltrim(a1));
    UART1_Write(Ltrim(b1));
    UART1_Write(Ltrim(c1));
    UART1_Write(Ltrim(d1));
    UART1_Write(Ltrim(e1));
    UART1_Write(Ltrim(f1));
    UART1_Write(Ltrim(g1));
    UART1_Write(Ltrim(h1));

}


void main() {
    TRISB.F0 = 0; // TRIGGER_RW as Output
    TRISB.F1 = 1; // ECHO_RW as Input
    TRISB.F2 = 0; // TRIGGER_DW as Output
    TRISB.F3 = 1; // ECHO_DW as Input
    TRISB.F4 = 0; // TRIGGER_RES as Output
    TRISB.F5 = 1; // ECHO_RES as Input
    TRISB.F6 = 0; // TRIGGER_UP as Output
    TRISB.F7 = 1; // ECHO_UP as Input
    TRISA.F0 = 0; // TRIGGER_DWN as Output
    TRISA.F1 = 1; // ECHO_DWN as Input
    TRISA.F2 = 0; // TRIGGER_Nutr as Output
    TRISA.F3 = 1; // ECHO_Nutr as Input

    TRISC.F0 = 1; // Push button
    TRISC.F1 = 0;
    TRISC.F2 = 0;
    TRISC.F4 = 0;
    TRISC.F5 = 0;
     
    UART1_Init(9600);

    T1CON = 0x10;
    ADCON1 = 0x0E;
    
    TRISD.F0 = 0;
    TRISD.F1 = 0;
    TRISD.RE0 = 1;
    TRISD.RE1 = 1;


    ADC_Init();
    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Delay_ms(100);
    Lcd_Out(1, 6, "System");
    Delay_ms(100);
    Lcd_Out(2, 4, "Starting...");
    Delay_ms(2000);
    Lcd_Cmd(_LCD_CURSOR_OFF);
    lcd_cmd(_lcd_clear);


    PB = 1;
    num = 0;

    while(1){
        waterlevel_RW();
        waterlevel_DW();
        waterlevel_Res();
        waterlevel_UP();
        waterlevel_DWN();
        waterlevel_Nutr();
        temperature_reading();
        pH_reading();

        display(num);
        push_button();
        controller();

        time_counter++;
        if (time_counter >= UART_TRANSMISSION_INTERVAL) {
            time_counter = 0;
            uart_transmission(distance_RW, distance_DW, distance_Res, distance_UP, distance_DWN, distance_Nutr, ph_value, tds_value);
        }
    }
}
