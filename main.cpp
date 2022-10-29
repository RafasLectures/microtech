/*
 * main.cpp
 *
 *  Created on: 26.10.2022
 *      Author: Rafael Andrioli Bauer
 */

#include <templateEMP.h>

#include <cstdint>
#include <chrono>
/*
 * Method to set the desired bits of a register to 1.
 * @param registerRef is the reference to the register to be accessed.
 *                    (the reference is like a const pointer, so we are accessing directly the
 *                    register here and not copying its value. One can think of it as const pointer
 *                    and when accessing this parameter inside of this function
 *                    the compiler automatically applies the * operator on the pointer.
 *                    For further explanation, I found this website
 *                    (https://www.geeksforgeeks.org/pointers-vs-references-cpp/#:~:text=Pointers%3A%20A%20pointer%20is%20a,for%20an%20already%20existing%20variable.)
 * @param bitSelection is the bits in which you want to set. E.g if you want to set bit4 of the register P1DIR you would call:
 *                     @code
 *                        setRegisterBits(P1DIR, 0x10);
 *                     @endcode
 * @tparam TYPE to enforce that one is setting the bitSelection according to the register size. More info bellow.
 *
 * The template was added so the the compiler can also do type checking to avoid unwanted behavior.
 * Like when you try to set a 16bit register with an 8bit bitMask, for example.
 *
 * One doesn't necessarily needs to define the template parameter, like setRegister<uint8_t>(P1DIR, 0x10),
 * the compiler is smart enough to deduce the template argument TYPE when using this function. So the compiler
 * enforces that both parameters are of the same size, like uint8_t or uin16_t...
 * More info in: https://en.cppreference.com/w/cpp/language/template_argument_deduction
 *
 * The constexpr was added so the compiler knows that it can try to resolve this in compile time.
 * It is kind of like a macro (#define), with the difference that one can get extra compiler checks. Constexpr functions can simply
 * replace the function call by the content of the function (like a macro does), but the compiler also checks:
 *      * if the parameter types are matching
 *      * if you would have access a non static address, it would actually not optimize it like a macro, because it would
 *        result in undefined behavior.
 * More info on constexpr can be found in https://en.cppreference.com/w/cpp/language/constexpr
 *
 * One nice thing to check is looking at the disassembly (in CCS View->Disassembly) with and without the constexpr, just to get a feeling. You can see
 * that many instructions are added when not having the constexpr specifier.
 */
template <typename TYPE>
constexpr void setRegisterBits(volatile TYPE& registerRef, TYPE bitSelection) noexcept {
    registerRef |= bitSelection;
}

/*
 * Method to set the desired bits of a register to 0.
 * @param registerRef is the reference to the register to be accessed. For info on reference check the documentation of setRegister
 * @param bitSelection is the bits in which you want to set to 0. E.g if you want to reset bit4 of the register P1DIR you would call:
 *                     @code
 *                        resetRegisterBits(P1DIR, 0x10);
 *                     @endcode
 * @tparam TYPE to enforce that one is setting the bitSelection according to the register size. More info bellow.
 *
 * For info on template or constexpr specifier, check the documentation of setRegister
 */
template <typename TYPE>
constexpr void resetRegisterBits(volatile TYPE& registerRef, TYPE bitSelection) noexcept {
    registerRef &= ~bitSelection;
}

/**
 * Method to toggle the desired bits of a register.
 * @param registerRef is the reference to the register to be accessed. For info on reference check the documentation of setRegister
 * @param bitSelection is the bits in which you want to toggle. E.g if you want to toggle bit4 of the register P1DIR you would call:
 *                     @code
 *                        toggleRegisterBits(P1DIR, 0x10);
 *                     @endcode
 * @tparam TYPE to enforce that one is setting the bitSelection according to the register size. More info bellow.
 * For info on template or constexpr specifier, check the documentation of setRegister
 */
template <typename TYPE>
constexpr void toggleRegisterBits(volatile TYPE& registerRef, TYPE bitSelection) noexcept {
    registerRef ^= bitSelection;
}


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
    // !! Duration is a type and periodValue is a value!!
    constexpr Duration period(periodValue);
    constexpr std::chrono::microseconds periodInUs = period;

    /*
     * Internal clock is 1 MHz, according to templateEMP.h
     *
     * So the compare value is basically the (period[us] / (1us * clockDiv)) - 1. The -1 because the counter starts in 0
     * Since all the the values are constants, the division is evaluated in compile time.
     */
    constexpr int64_t compareValue = (periodInUs.count() / CLK_DIV) - 1;

    // Static assert so if the compareValue is bigger than 0xFFFF the compiler gives an error. This is not added as instructions in the binary.
    // One could verify that it works by calling "setupTimer0<5, std::chrono::seconds>();".
    static_assert((compareValue < 0xFFFF), "Cannot set desired timer period. It exceeds the counter maximum value");

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



/**
constexpr void setIOsAtPortAsOutput(const uint8_t port, uint16_t bitSelection) noexcept {
    if (port == 1) {

    } else if (port == 2) {
        setIOsAsOutput(P2DIR, P2SEL, P2SEL2, static_cast<uint8_t>(bitSelection));
    } else if (port == 3) {
        setIOsAsOutput(P3DIR, P3SEL, P3SEL2, static_cast<uint8_t>(bitSelection));
    }
}
*/

//Timer0 Interruption
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_CCR0_ISR(void)
{
    toggleRegisterBits(P1OUT, static_cast<uint8_t>(BIT4));
}

void main() {

    initMSP();

    // Initialize all  outputs of port 1 to 0
    resetRegisterBits(P1OUT, static_cast<uint8_t>(0xFF));
    // Disable all internal resistors of the port
    resetRegisterBits(P1REN, static_cast<uint8_t>(0xFF));

    setIOsAsOutput(P1DIR, P1SEL, P1SEL2, static_cast<uint8_t>(BIT4 | BIT5));

    //setIOsAtPortAsOutput(1, BIT4);
    //setPinAsOutput(IOPins::CON3_P1_4);

    //setRegisterBits(P1OUT, static_cast<uint8_t>(BIT5));
    //resetRegisterBits(P1OUT, static_cast<uint8_t>(BIT4));

    // set timer interruption for 500ms
    setupTimer0<500, std::chrono::milliseconds>();
    _enable_interrupt();

    while(true) {}

}

