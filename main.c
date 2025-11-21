/*
 * Project: Smart Greenhouse Controller
 * Fix: Pump moved to RC2 to avoid RA4 Open Drain issue
 */

#include <xc.h>
#include <stdio.h>

// Configuration bits
#pragma config FOSC = HS        // High Speed Crystal
#pragma config WDTE = OFF       // Watchdog Timer Off
#pragma config PWRTE = ON       // Power-up Timer On
#pragma config BOREN = ON       // Brown-out Reset On
#pragma config LVP = OFF        // Low Voltage Programming Off
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection Off
#pragma config WRT = OFF        // Flash Program Memory Write Enable Off
#pragma config CP = OFF         // Flash Program Memory Code Protection Off

#define _XTAL_FREQ 20000000

// LCD Pins
#define LCD_E   RB0
#define LCD_RS  RB1  
#define LCD_RW  RB2
#define LCD_D4  RB4
#define LCD_D5  RB5
#define LCD_D6  RB6
#define LCD_D7  RB7

// Control Pins
#define GREEN_LED   RA0
#define RED_LED     RA1  
#define BUZZER      RA2
#define FAN_RELAY   RA3    // Fan on RA3 (Pin 5)
#define PUMP_RELAY  RC2    // *** CHANGED TO RC2 (Pin 13) ***

// SHT75 Pins
#define SHT_DATA    RC1
#define SHT_CLK     RC0

// Limits
#define MAX_TEMP    30     
#define MIN_TEMP    18     
#define MAX_HUMID   70     
#define MIN_HUMID   40     // Pump turns ON if below 40%

// UART Init for Python
void uart_init() {
    TRISC6 = 0; // TX Output
    TRISC7 = 1; // RX Input
    SPBRG = 129; // 9600 Baud
    TXSTA = 0x24; 
    RCSTA = 0x90; 
}

void putch(char data) {
    while(!TRMT);
    TXREG = data;
}

// LCD Functions
void lcd_nibble(unsigned char n) {
    LCD_D4 = (n & 1) ? 1 : 0;
    LCD_D5 = (n & 2) ? 1 : 0;
    LCD_D6 = (n & 4) ? 1 : 0;
    LCD_D7 = (n & 8) ? 1 : 0;
    LCD_E = 1; __delay_us(10); LCD_E = 0; __delay_us(50);
}

void lcd_cmd(unsigned char c) {
    LCD_RS = 0; LCD_RW = 0;
    lcd_nibble(c >> 4); lcd_nibble(c & 0x0F);
    if(c == 0x01 || c == 0x02) __delay_ms(2);
}

void lcd_data(unsigned char d) {
    LCD_RS = 1; LCD_RW = 0;
    lcd_nibble(d >> 4); lcd_nibble(d & 0x0F);
    __delay_us(50);
}

void lcd_init() {
    LCD_E = 0; LCD_RS = 0; LCD_RW = 0;
    __delay_ms(50);
    lcd_nibble(0x03); __delay_ms(5);
    lcd_nibble(0x03); __delay_us(150);
    lcd_nibble(0x03); lcd_nibble(0x02);
    lcd_cmd(0x28); lcd_cmd(0x0C); lcd_cmd(0x06); lcd_cmd(0x01);
    __delay_ms(5);
}

void lcd_puts(const char *s) { while(*s) lcd_data(*s++); }

void lcd_pos(unsigned char line, unsigned char pos) {
    unsigned char addr = (line == 0) ? 0x80 : 0xC0;
    lcd_cmd(addr + pos);
}

void lcd_number(unsigned char num) {
    lcd_data((num / 10) + '0');
    lcd_data((num % 10) + '0');
}

// SHT75 Functions
void sht_start() {
    TRISC1 = 0; SHT_DATA = 1; SHT_CLK = 0;
    TRISC1 = 1; SHT_CLK = 1; __delay_us(1);
    TRISC1 = 0; SHT_DATA = 0; __delay_us(1);
    SHT_CLK = 0; __delay_us(2); SHT_CLK = 1; __delay_us(1);
    TRISC1 = 1; __delay_us(1); SHT_CLK = 0;
}

unsigned char sht_write(unsigned char data) {
    unsigned char i, mask = 0x80, ack;
    for(i = 0; i < 8; i++) {
        SHT_CLK = 0;
        if(data & mask) TRISC1 = 1; else { TRISC1 = 0; SHT_DATA = 0; }
        __delay_us(1); SHT_CLK = 1; __delay_us(1); mask >>= 1;
    }
    SHT_CLK = 0; TRISC1 = 1; __delay_us(1);
    SHT_CLK = 1; __delay_us(1); ack = SHT_DATA; SHT_CLK = 0;
    return ack;
}

unsigned int sht_read() {
    unsigned char i; unsigned int data = 0;
    TRISC1 = 1; 
    for(i = 0; i < 8; i++) {
        data <<= 1; SHT_CLK = 1; __delay_us(1);
        if(SHT_DATA) data |= 1; SHT_CLK = 0; __delay_us(1);
    }
    TRISC1 = 0; SHT_DATA = 0; SHT_CLK = 1; __delay_us(2); SHT_CLK = 0; TRISC1 = 1;
    for(i = 0; i < 8; i++) {
        data <<= 1; SHT_CLK = 1; __delay_us(1);
        if(SHT_DATA) data |= 1; SHT_CLK = 0; __delay_us(1);
    }
    TRISC1 = 1; SHT_CLK = 1; __delay_us(2); SHT_CLK = 0;
    return data;
}

void sht_wait() {
    unsigned long timeout = 30000;
    TRISC1 = 1; SHT_CLK = 0;
    while(timeout-- && SHT_DATA) __delay_us(5);
}

void sht_read_values(unsigned char *temp, unsigned char *humid) {
    unsigned int st, sh;
    sht_start(); if(sht_write(0x03) == 0) { sht_wait(); st = sht_read(); } else st = 0;
    sht_start(); if(sht_write(0x05) == 0) { sht_wait(); sh = sht_read(); } else sh = 0;

    float temp_f = -40.01 + 0.01 * (float)st;
    float rh_lin = -4.0 + 0.0405 * (float)sh - 2.8e-6 * (float)sh * (float)sh;
    float humid_f = (temp_f - 25.0) * (0.01 + 0.00008 * (float)sh) + rh_lin;

    if(humid_f > 99) humid_f = 99; if(humid_f < 0) humid_f = 0;
    *temp = (unsigned char)temp_f; *humid = (unsigned char)humid_f;
}

void sht_init() {
    TRISC0 = 0; TRISC1 = 1; 
    for(unsigned char i = 0; i < 9; i++) { SHT_CLK = 1; __delay_us(2); SHT_CLK = 0; __delay_us(2); }
    sht_start(); __delay_ms(20);
}

// Main
void main() {
    unsigned char temp = 0, humid = 0;
    
    CMCON = 0x07;   // Comparators OFF
    ADCON1 = 0x06;  // PORTA Digital
    
    TRISA = 0x00;   // All Output
    TRISB = 0x00;   // All Output
    TRISC = 0x00;   // All Output (SHT/UART will override automatically)
    
    GREEN_LED = 0; RED_LED = 0; BUZZER = 0;
    FAN_RELAY = 0; PUMP_RELAY = 0;
    
    lcd_init();
    sht_init();
    uart_init();
    
    lcd_cmd(0x01); lcd_puts("System Init...");
    __delay_ms(1000);

    while(1) {
        sht_read_values(&temp, &humid);
        
        // --- CONTROL LOGIC ---
        
        // 1. Fan Logic
        if(temp > MAX_TEMP) FAN_RELAY = 1;
        else if(temp < MIN_TEMP) FAN_RELAY = 0;
        
        // 2. Pump Logic (Inverted: Low humidity = Pump ON)
        if(humid < MIN_HUMID) {
            PUMP_RELAY = 1; // Turn ON
        } else if(humid > MAX_HUMID) {
            PUMP_RELAY = 0; // Turn OFF
        }
        
        // 3. Alarm Logic
        if(temp > MAX_TEMP + 5 || humid < MIN_HUMID - 10) {
            GREEN_LED = 0; RED_LED = 1; BUZZER = 1;
        } else {
            GREEN_LED = 1; RED_LED = 0; BUZZER = 0;
        }
        
        // --- DISPLAY LOGIC ---
        lcd_pos(0, 0); lcd_puts("T:"); lcd_number(temp); lcd_puts("C ");
        if(FAN_RELAY) lcd_puts("FAN:ON "); else lcd_puts("FAN:OFF");
        
        lcd_pos(1, 0); lcd_puts("H:"); lcd_number(humid); lcd_puts("% ");
        if(PUMP_RELAY) lcd_puts("PMP:ON "); else lcd_puts("PMP:OFF");
        
        // Send Data to Python
        printf("TEMP:%d,HUM:%d\r\n", temp, humid);
        
        __delay_ms(1000);
    }
}