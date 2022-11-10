/***************************************************************************//**
 * @file                    exercise1.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    26.10.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 1 - LED blink and serial communication
 *
 * Description: write about your code
 *
 * Pin connections: mention all the pin connections
 *
 * Theory answers: if required. With question number.
 *
 * Tasks completed:
 *  Task 1
 *      a) [x] using P1.4                    (1.0/1.0 pt.)
 *         [x] blinks correctly              (1.0/1.0 pt.)
 *         [x] with 2 Hertz                  (1.0/1.0 pt.)
 *      b) [x] using P1.5                    (1.0/1.0 pt.)
 *         [x] half of the blinking freq.    (1.0/1.0 pt.)
 *         [x] description for X10           (0.75/1.0 pt.)
 *      c) [x] serialAvailable/serialRead    (1.0/1.0 pt.)
 *         [x] serialFlush/serialRead        (1.0/1.0 pt.)
 *         [x] returning the state           (1.0/1.0 pt.)
 *
 *  Task 2
           [ ] feedback.txt                  (0.0/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/


#include <templateEMP.h>

#include "helpers"

#include <cstdint>
#include <chrono>

/**
 * Method implements the sequence to set one or multiple IOs as an output.
 *  1. Set PxDIR to 1
 *  2. Set PxSEL to 0
 *  3. Set PxSEL2 to 0
 *
 * @param PxDir reference to PxDIR register
 * @param PxSel reference to PxSEL register
 * @param PxSel reference to PxSEL2 register
 * @param bitSelection Is the pins of the port that you want to set as an output. So if you want to set pin 4 as an output, then the bitSelection would be 0x10.
 *                     For pin 4 and 1, it would be 0x12.
 */
constexpr void setIOsAsOutput(volatile uint8_t& PxDir, volatile uint8_t& PxSel, volatile uint8_t& PxSel2, uint8_t bitSelection) noexcept {
    setRegisterBits(PxDir, bitSelection);
    resetRegisterBits(PxSel, bitSelection);
    resetRegisterBits(PxSel2, bitSelection);
}

/**
 * Method to setup the timer 0. It will enable the interrupt of timer 0, but one must call _enable_interrupt() at some other time
 * for the interruption start to happen.
 * It is an easy interface to setup the timer to a desired interrupt period. E.g.:
 *
 * @code
 *  // Set to 500ms
 *  setupTimer0<500, std::chrono::milliseconds>();
 *
 *  // Set to 1s
 *  setupTimer0<1, std::chrono::seconds>();
 *
 *  // Set to 50us
 *  setupTimer0<50, std::chrono::microseconds>();
 * @endcode
 *
 * @tparam periodValue The period value in that specific magnitude.
 * @tparam Duration std::chrono duration type.
 *
 * The whole logic ad calculation is done in compile time. The only thing that goes to the binary is the setting of the registers.
 */
template<uint64_t periodValue, typename Duration = std::chrono::microseconds>
constexpr void setupTimer0() {
    // If one wants to change this, then you also need to change the
    // TACTL ID_x bits
    // Maybe a future improvement would be to calculate the DIV bits at compile time as well...
    // Declared it int64_t, because it is used for the compare value calculation, and the
    // std::chrono::microseconds::count() returns an int64_t
    constexpr int64_t CLK_DIV = 8;

    // Declares the duration given by the user and then converts it to microseconds
    // !! Duration is a type and periodValue is a value !!
    constexpr Duration period(periodValue);
    constexpr std::chrono::microseconds periodInUs = period;

    /*
     * Internal clock is 1 MHz, according to templateEMP.h
     *
     * So the compare value is basically the (period[us] / (1us * clockDiv)) - 1. The -1 because the counter starts in 0
     * !!!! Since all the the values are constants, the division is evaluated in compile time. !!!!!
     */
    constexpr int64_t compareValue = (periodInUs.count() / CLK_DIV) - 1;

    // Static assert so if the compareValue is bigger than 0xFFFF the compiler gives an error. This is not added as instructions in the binary.
    // One could verify that it works by calling "setupTimer0<5, std::chrono::seconds>();".
    static_assert((compareValue <= 0xFFFF), "Cannot set desired timer period. It exceeds the counter maximum value");

    /* Set Timer 0 compare value. Casted to 16 bits due to register size.
     * One can see that it is the correct value when looking at the disassembly on the call
     * setupTimer0<500, std::chrono::milliseconds>();. the assembly command is
     * MOV.W   #0xf423,&Timer0_A3_TA0CCR0. Where #0xf423 is the value coming from compareValue evaluated in compile time
     * which is correct: (500000/8)-1 = 62499 = 0xF423.
     */
    TACCR0 = static_cast<uint16_t>(compareValue);
    // Enable interrupt for CCR0.
    TACCTL0 |= CCIE;
    // Choose SMCLK as clock source
    // Clock divided by 8
    // Counting in Up Mode
    TACTL = TASSEL_2 + ID_3 + MC_1;
}



void main() {

    initMSP();

    // Initialize all  outputs of port 1 to 0
    resetRegisterBits(P1OUT, static_cast<uint8_t>(0xFF));
    // Disable all internal resistors of the port
    resetRegisterBits(P1REN, static_cast<uint8_t>(0xFF));

    /*
     * Pin X10 can also be used to activate a heater (in the circuit represented by a resistor)
     * It polarizes the transistor T2, which connects the heater resistor to ground.
     * For it to function correctly, the jumper JP4 must be connected between pin 1 and 2.
     */
    setIOsAsOutput(P1DIR, P1SEL, P1SEL2, static_cast<uint8_t>(BIT4 | BIT5));

    // Initialize local variable that stores the value of Pin P1.5
    uint8_t pin5State = getRegisterBits(P1OUT, static_cast<uint8_t>(BIT5), static_cast<uint8_t>(5));

    // set timer interruption for 250ms
    setupTimer0<250, std::chrono::milliseconds>();
    _enable_interrupt();

    // Infinite loop
    while(true) {
        if(serialAvailable()) {
            // Need to make sure that there are no more bytes comming
            // Loop while there are bytes available in the serial
            while(serialAvailable()) {
                // Flush the serial
                serialFlush();

                // Add a delay to give time if there are more bytes "coming"
                // Serial Baudrate is 9600 bits/s
                // 1s is 1.000.000 cycles, since one cycle is 1us.
                // Calculate cycles per bit:
                // 1.000.000 / 9600 = 104,16 cycles/bit
                // 104,16 * 8 = 833,33 cycles/byte.
                // Add some more cycles just to be safe.
                __delay_cycles(1500);
            }
            // Get value from register and store in local variable to prevent access from interrupt and while loop
            pin5State = getRegisterBits(P1OUT, static_cast<uint8_t>(BIT5), static_cast<uint8_t>(5));
            if(pin5State) {
                serialPrint("LED D7 is ON\n");
            } else {
                serialPrint("LED D7 is OFF\n");
            }

        }
    }

}
