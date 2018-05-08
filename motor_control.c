#include <stdio.h>
#include <p18f4525.h>
#include "sumovore.h"
#include "motor_control.h"

#define __1_SECOND__ 31250 // 4/32000000*256*31250 = 1.00s
#define __180TURN__ 19375 // 0.62s
#define __90TURN__ 8000 // 0.21s
#define __30TURN__ 2667 // 0.07s
#define __LANDINGPADCHECK__ 250 //0.24s
#define __BLINKRATE__ 6500 //0.24s


void follow_simple_curves(void);

void straight_fwd(void);
void spin_left(void);
void turn_left(void);
void turn_right(void);
void spin_right(void);
void turn_90_degrees_right(void);
void turn_90_degrees_left(void);
void turn_30_degrees_right(void);
void turn_30_degrees_left(void);
void turn_180_degrees(void);
void big_left_curve(void);
void big_right_curve(void);
void play_with_LEDs(void);
void landing_pad(void);

void motor_control(void) {
    // very simple motor control
    switch (SeeLine.B) {

        case 0b11111u: //play_with_LEDs();
            landing_pad();
            break;
        case 0b00000u: turn_180_degrees();
            break;
        case 0b00111u:
        case 0b01111u: turn_90_degrees_right();
            break;
        case 0b11100u:
        case 0b11110u: turn_90_degrees_left();
            break;
        case 0b10100u: turn_30_degrees_left();
            break;
        case 0b00101u: turn_30_degrees_right();
            break;
        case 0b01100u:
        case 0b11000u:
            big_left_curve();
            break;
        case 0b00110u:
        case 0b00011u:
            big_right_curve();
            break;
        case 0b00100u:
        case 0b00010u:
        case 0b01000u:
        case 0b00001u:
        case 0b10000u:
            //no breaks all above readings end up here
            follow_simple_curves();
            break;

        default:
        {
            spin_right();
            check_sensors();
        }
    }
}

void follow_simple_curves(void) {
    if (SeeLine.b.Center) straight_fwd();
    else if (SeeLine.b.Left) spin_left();
    else if (SeeLine.b.CntLeft) turn_left();
    else if (SeeLine.b.CntRight) turn_right();
    else if (SeeLine.b.Right) spin_right();
}

void straight_fwd(void) {
    set_motor_speed(left, slow, 0);
    set_motor_speed(right, slow, 0);
}

void spin_left(void) {
    set_motor_speed(left, rev_slow, 0);
    set_motor_speed(right, slow, 0);
}

void spin_right(void) {
    set_motor_speed(left, slow, 0);
    set_motor_speed(right, rev_slow, 0);
}

void turn_left(void) {
    set_motor_speed(left, stop, 0);
    set_motor_speed(right, slow, 0);
}

void turn_right(void) {
    set_motor_speed(left, slow, 0);
    set_motor_speed(right, stop, 0);
}

void turn_180_degrees(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_256);

    WriteTimer0(0); // Sees white space, continues forward for a while
    while (ReadTimer0() < __1_SECOND__ / 2 && SeeLine.B == 0b00000) { // 4/32Mhz * 256 * 13125 = 0.42s
        straight_fwd();
        check_sensors();
        if (SeeLine.B != 0b00000u)return;
    }

    WriteTimer0(0); // Turns 180 degrees
    while (ReadTimer0() < __180TURN__ * 1 / 1.4) { // 4/32Mhz * 256 * 45625 = 0.50s
        set_motor_speed(left, rev_medium, 0);
        set_motor_speed(right, medium, 0);
    }

    WriteTimer0(0); // heads back to path
    while (ReadTimer0() < __1_SECOND__ && SeeLine.B == 0b00000u) { // 4/32Mhz * 256 * 13125 = 0.42s
        straight_fwd();
        check_sensors();
        if (SeeLine.B != 0b00000u)return;
    }
}

void turn_30_degrees_left(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_128);
    TMR0IF = 0;
    WriteTimer0(0);
    while (!SeeLine.B == 0b00000u) {
        straight_fwd();
        check_sensors();
        set_leds();
        if (TMR0IF == 1)return;
    }
    WriteTimer0(0);
    while ((SeeLine.B && 0b00100u != 0b00100u) && ReadTimer0() < __1_SECOND__) {
        check_sensors();
        spin_left();
        check_sensors();
        if (TMR0IF == 1)return;
    }

    if (SeeLine.B == 0b01000u) return;
}

void turn_30_degrees_right(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_128);
    TMR0IF = 0;
    WriteTimer0(0);
    while (!SeeLine.B == 0b00000u) {
        straight_fwd();
        check_sensors();
        set_leds();
        if (TMR0IF == 1)return;
    }
    WriteTimer0(0);
    while ((SeeLine.B && 0b00100u != 0b00100u) && ReadTimer0() < __1_SECOND__) {
        check_sensors();
        spin_left();
        check_sensors();
    }

    if (SeeLine.B == 0b00010u) return;
}

void turn_90_degrees_left(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_256);
    WriteTimer0(0); // Sees white space, continues forward for 1 second
    while (ReadTimer0() < __1_SECOND__) {
        check_sensors();
        turn_right();
        if (SeeLine.B == 0b00000u) {
            motors_brake_all();
            for (int i = 0; i < 500; i++) _delay(8000);
            WriteTimer0(0); // Sees white space, continues forward for 1 second
            while (ReadTimer0() < __90TURN__)spin_left();
            motors_brake_all();
            for (int i = 0; i < 500; i++) _delay(8000);
            return;
        } else if (SeeLine.B == 0b00100u) return;
    }
}

void turn_90_degrees_right(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_256);
    WriteTimer0(0); // Sees white space, continues forward for 1 second
    while (ReadTimer0() < __1_SECOND__) {
        check_sensors();
        turn_left();
        if (SeeLine.B == 0b00000u) {
            motors_brake_all();
            for (int i = 0; i < 500; i++) _delay(8000);
            WriteTimer0(0);
            while (ReadTimer0() < __90TURN__)spin_right();
            motors_brake_all();
            for (int i = 0; i < 500; i++) _delay(8000);
            return;
        } else if (SeeLine.B == 0b00100u) return;
    }
}

void big_left_curve() {
    set_motor_speed(left, rev_medium, 0);
    set_motor_speed(right, fast, 0);
}

void big_right_curve() {
    set_motor_speed(left, fast, 0);
    set_motor_speed(right, rev_medium, 0);
}

void landing_pad(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_256);
    WriteTimer0(0);
    while (ReadTimer0() < (__LANDINGPADCHECK__));
    check_sensors();
    if ((ReadTimer0() >= __LANDINGPADCHECK__) && ((SeeLine.B == 0b11111) || (SeeLine.B == 0b01111) || (SeeLine.B == 0b11110) || (SeeLine.B == 0b01110))) {
        motors_brake_all();
    }

/*
void landing_pad(void) {
    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_256);
    WriteTimer0(0);
    while (ReadTimer0() < (__LANDINGPADCHECK__)) {
        check_sensors();
        if (SeeLine.B != 0b11111)return;
    }
    if (SeeLine.B == 0b11111)
            motors_brake_all();
            for (int i = 0; i < 10000; i++) _delay(8000);
}
*///Landing pad 2

void play_with_LEDs(void) {

    OpenTimer0(TIMER_INT_OFF & T0_SOURCE_INT & T0_16BIT & T0_PS_1_256);

    WriteTimer0(0); // Sees white space, continues forward for 1 second
    while (ReadTimer0() < 6000) {

        if (ReadTimer0() <= 2000) {
            setLED1(0);
            setLED2(0);
            setLED3(1);
            setLED4(0);
            setLED5(0);
        }

        if (ReadTimer0() > 2000 && ReadTimer0() <= 4000) {

            setLED1(0);
            setLED2(1);
            setLED3(0);
            setLED4(1);
            setLED5(0);
        }

        if (ReadTimer0() > 4000) {

            setLED1(1);
            setLED2(0);
            setLED3(0);
            setLED4(0);
            setLED5(1);
        }
        check_sensors();
        if (SeeLine.B == 0b00000u)break;
    }
}

