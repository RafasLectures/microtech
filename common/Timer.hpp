/******************************************************************************
 * @file                    Timer.hpp
 * @author                  Rafael Andrioli Bauer
 * @date                    09.11.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Header contains abstraction of GPIOs
 *
 * Description: This file is still work in progress, but its goal is described below.
 *              In order to abstract the manipulation of registers and
 *              ease up maintainability of the application code, this header
 *              provides an easy interface to setup a timer.
 *              It allows the user to set callback functions for periodic
 *              wakeu-ps or a one time wake-up
 *
 *              The interface was developed using template classes as well as
 *              constexpr so the program memory usage would be optimized.
 ******************************************************************************/

#ifndef COMMON_TIMER_HPP_
#define COMMON_TIMER_HPP_

#include "helpers.hpp"

#include <msp430g2553.h>
#include <cstdint>
#include <chrono>
#include <array>


namespace Microtech {

class TaskHandlerBase {
public:
    constexpr TaskHandlerBase(void (*callback)(), bool isPeriodic) : taskCallback(callback), isPeriodic(isPeriodic) {}

    inline void callCallback() {
        taskCallback();
    }

private:
    void (*taskCallback)();
    bool isPeriodic;
};


template <uint64_t periodValue, typename Duration = std::chrono::microseconds>
class TaskHandler : public TaskHandlerBase {
public:
    constexpr TaskHandler(void (*callback)(), bool isPeriodic) : TaskHandlerBase(callback,isPeriodic) {}
};

template <int64_t CLK_DIV>
class Timer {
public:
    Timer() = default;
    ~Timer() = default;

    constexpr void init() {
        constexpr uint16_t timerInputDivider = getTimerInputDivider();
        // Choose SMCLK as clock source
        // Counting in Up Mode
        TACTL = TASSEL_2 + timerInputDivider + MC_0;
    }
    /**
     * Method to register a task to the timer. It will enable the interrupt of timer 0,
     * but one must call _enable_interrupt() at some other time
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
    constexpr void registerTask(TaskHandler<periodValue, Duration>& task) {
        constexpr uint16_t compareValue = calculateCompareValue<periodValue, Duration>();

        // For now only one task this is work in progress.
        taskHandlers[0] = &task;

        /* Set Timer 0 compare value.
         * One can see that it is the correct value when looking at the disassembly on the call
         * setupTimer0<500, std::chrono::milliseconds>();. the assembly command is
         * MOV.W   #0xf423,&Timer0_A3_TA0CCR0. Where #0xf423 is the value coming from compareValue evaluated in compile time
         * which is correct: (500000/8)-1 = 62499 = 0xF423.
         */
        TACCR0 = compareValue;
        // Enable interrupt for CCR0.
        TACCTL0 |= CCIE;
        setRegisterBits(TACTL, static_cast<uint16_t>(MC_1));
    }


    bool deregisterTask(TaskHandlerBase& taskHandler) {

    }


    inline void interruptionHappened() {
        taskHandlers[0]->callCallback();
    }
private:
    static constexpr uint16_t getTimerInputDivider() {
        switch (CLK_DIV) {
        case 1: return ID_0;
        case 2: return  ID_1;
        case 4: return  ID_2;
        case 8: return  ID_3;
        default:
            static_assert(CLK_DIV == 1 || CLK_DIV == 2 || CLK_DIV == 4 || CLK_DIV == 8, "Timer input divider is invalid. Must be either 1, 2, 4 or 8");
        }
        return ID_0; // It will actually never get here. But it is needed due to the compiler warning
    }

    template<uint64_t periodValue, typename Duration = std::chrono::microseconds>
    static constexpr uint16_t calculateCompareValue() {
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
        return static_cast<uint16_t>(compareValue);
    }

    std::array<TaskHandlerBase*, 5> taskHandlers;
};

} /* namespace Microtech */

#endif /* COMMON_TIMER_HPP_ */
