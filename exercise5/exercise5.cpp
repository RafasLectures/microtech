/*****************************************************************************
 * @file                    exercise5.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    07.12.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 5 - Pulse Width Modulation
 *
 * Description:
 *
 * Pin connections:  BUZZER <-> CON3:P3.6
 *                   PB5 <-> CON3:P1.3
 *                   PB6 <-> CON3:P1.4
 *                   JP5 to VFO
 *
 * Theory answers: None.
 *
 * Tasks completed:
 *  Task 1
 *    a)   [ ] PWM with 50% duty-cycle            (x/1,0 pt.)
 *         [ ] Store melody in array              (x/1,0 pt.)
 *         [ ] Melody 1                           (x/1,0 pt.)
 *         [ ] Melody 2                           (x/1,0 pt.)
 *   b)
 *         [ ] Capture PB5 with interrupt         (x/1,0 pt.)
 *         [ ] Press once play melody one         (x/1,0 pt.)
 *         [ ] Press twice play melody two        (x/1,0 pt.)
 *   c)
 *         [ ] Detect board knock from piezo P3IN (x/2,0 pt.)
 *         [ ] Press twice play melody two        (x/1,0 pt.)
 *   d)
 *         [ ] PB6 as a pause/resume button       (x/1,0 pt.)
 *
 *  Task 2
 *         [ ] feedback.txt                       (x/1.0 pt.)
 *
 * @note    The project was exported using CCS 12.1.0.00007
 ******************************************************************************/
#include <templateEMP.h>

#include "GPIOs.hpp"
#include "Timer.hpp"

#include "Pwm.hpp"

#include <chrono>

#define CAPTURE_FROM_PB5

using namespace Microtech;
Pwm pwm(GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(6)>());

#ifdef CAPTURE_FROM_PB5
constexpr InputHandle pb5 = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(3)>();
constexpr InputHandle pb6 = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(4)>();
#endif

typedef void (Pwm::*PwmFuncPointer)();

constexpr PwmFuncPointer NONE = &Pwm::setPwmPeriod<0>;
constexpr PwmFuncPointer C4 = &Pwm::setPwmPeriod<3822>;
constexpr PwmFuncPointer D4 = &Pwm::setPwmPeriod<3405>;
constexpr PwmFuncPointer E4 = &Pwm::setPwmPeriod<3033>;
constexpr PwmFuncPointer F4 = &Pwm::setPwmPeriod<2863>;
constexpr PwmFuncPointer G4 = &Pwm::setPwmPeriod<2551>;
constexpr PwmFuncPointer A4 = &Pwm::setPwmPeriod<2272>;
constexpr PwmFuncPointer Bb4 = &Pwm::setPwmPeriod<2145>;
constexpr PwmFuncPointer B4 = &Pwm::setPwmPeriod<2024>;
constexpr PwmFuncPointer C5 = &Pwm::setPwmPeriod<1911>;


struct NoteWithTempo {
    PwmFuncPointer note;
    uint8_t numTicks;
};

struct Playlist {
    const size_t musicSize;
    const NoteWithTempo* firstNote;
};

constexpr NoteWithTempo WeWishYouAMerryChristmas[] {
    {C4, 2},
    {F4, 2},
    {F4, 1},
    {G4, 1},
    {F4, 1},
    {E4, 1},
    {D4, 2},
    {D4, 2},
    {D4, 2},
    {G4, 2},
    {G4, 1},
    {A4, 1},
    {G4, 1},
    {F4, 1},
    {E4, 2},
    {C4, 2},
    {C4, 2},
    {A4, 2},
    {A4, 1},
    {Bb4, 1},
    {A4, 1},
    {G4, 1},
    {F4, 2},
    {D4, 2},
    {C4, 1},
    {C4, 1},
    {D4, 2},
    {G4, 2},
    {E4, 2},
    {F4, 4},
    {NONE, 8}
};

constexpr NoteWithTempo JingleBells[] {
    {A4, 2},
    {A4, 2},
    {A4, 4},
    {A4, 2},
    {A4, 2},
    {A4, 4},
    {A4, 2},
    {C5, 2},
    {F4, 3},
    {G4, 1},
    {A4, 8},
    {NONE, 2},
    {Bb4, 2},
    {Bb4, 2},
    {Bb4, 3},
    {Bb4, 1},
    {Bb4, 2},
    {A4, 2},
    {A4, 2},
    {A4, 1},
    {A4, 1},
    {C5, 2},
    {C5, 2},
    {Bb4, 2},
    {G4, 2},
    {F4, 8},
    {NONE, 8}
};

constexpr Playlist playlist[]{
  {sizeof(WeWishYouAMerryChristmas)/sizeof(WeWishYouAMerryChristmas[0]), &WeWishYouAMerryChristmas[0]},
  {sizeof(JingleBells)/sizeof(JingleBells[0]), &JingleBells[0]},
};

bool startFromBeginning = true;
uint8_t musicIndex = 0;

#ifdef CAPTURE_FROM_PB5

bool play = false;
bool countNumberOfPressPb5 = false;
uint8_t numberOfPressPb5 = 0;

void pauseResume() {
    //play = true;
}

bool pb5Pressed = false;
void melodySelection() {
    if(countNumberOfPressPb5 == false) {
        numberOfPressPb5 = 0;
    }
    countNumberOfPressPb5 = true;
    pb5Pressed = true;
}

void countNumberOfPressesPB5() {
    static constexpr uint8_t TICKS_FOR_1_SECOND = 8;
    static uint8_t buttonTime = 0;

    if(buttonTime > TICKS_FOR_1_SECOND){
        countNumberOfPressPb5 = false;
        buttonTime = 0;

        if(numberOfPressPb5 > 2) {
            musicIndex = 1;
        }else {
            musicIndex = 0;
        }
        play = true;
        startFromBeginning = true;
        pb5.disableInterrupt();
    } else {
        buttonTime++;
        if(pb5Pressed == true){
            numberOfPressPb5++;
            pb5Pressed = false;
        }
    }
}
#else
constexpr bool play = true;
bool startFromBeginning = true;

#endif

void playTaskFunc() {
    constexpr size_t PLAYLIST_SIZE = sizeof(playlist)/sizeof(playlist[0]);
    static uint8_t noteIndex = 0xFA;
    static uint8_t remainingTicks = 0;
    static const NoteWithTempo* currentNote = playlist[musicIndex].firstNote;

    if(play) {
        if(startFromBeginning == true) {
            currentNote = playlist[musicIndex].firstNote;
            remainingTicks = currentNote->numTicks;
            noteIndex = 0;
            (pwm.*(currentNote->note))();
            startFromBeginning = false;
        }

        if(remainingTicks == 0) {
            pwm.setPwmPeriod<0>();

            noteIndex++;
            currentNote++;

            if(noteIndex >= playlist[musicIndex].musicSize) {
    #ifndef CAPTURE_FROM_PB5
                musicIndex++;
                if(musicIndex >= PLAYLIST_SIZE) {
                    musicIndex = 0;
                }
                noteIndex = 0;
                currentNote =  playlist[musicIndex].firstNote;
    #else
                play = false;
                pwm.stop();
                pb5.enableInterrupt();
    #endif
            }
            if(play) {
                remainingTicks = currentNote->numTicks;
                (pwm.*(currentNote->note))();
            }
        } else {
            remainingTicks--;
        }
    }

#ifdef CAPTURE_FROM_PB5
    if(countNumberOfPressPb5 == true) {
        countNumberOfPressesPB5();
    }
#endif
}

int main() {
  initMSP();
  constexpr TimerConfigBase<8> TIMER_CONFIG;
  Timer<1>::getTimer().init(TIMER_CONFIG);

#ifdef CAPTURE_FROM_PB5
  pb5.init();
  pb6.init();

  pb5.enableInterrupt();
  pb6.enableInterrupt();
#endif

  pwm.init();
  pwm.setDutyCycle(500);

  // Creates a 250ms periodic task for evaluating the adc values.
  TaskHandler<125, std::chrono::milliseconds> playTask(&playTaskFunc, true);

  // registers adc task to timer 0
  Timer<1>::getTimer().registerTask(TIMER_CONFIG, playTask);

  // globally enables the interrupts.
  __enable_interrupt();

  while (true) {
    _no_operation();
  }
  return 0;
}

#ifdef CAPTURE_FROM_PB5
// Port 1 interrupt vector
#pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR(void) {

  // Get bits 3 and 4 => 0001 1000 = 0x18
  const uint8_t pendingInterrupt = getRegisterBits(P1IFG, static_cast<uint8_t>(0x18), static_cast<uint8_t>(3));

  if(pendingInterrupt > 1) {
      pauseResume();
  } else {
      melodySelection();
  }

  // Sets the state of the button based on the button state evaluation.
  // If the button state is different, internally the button calls its callback which calls
  // the state machine evaluation.
//  pb5.setState(pb5CurrentState);

  const uint8_t clearFlag = (pendingInterrupt << 0x03);
  resetRegisterBits(P1IFG, clearFlag);  // clear interrupt flag
}
#endif
