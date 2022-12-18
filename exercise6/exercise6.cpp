/*****************************************************************************
* @file                    exercise6.cpp
* @author                  Rafael Andrioli Bauer
* @date                    18.12.2022
* @matriculation number    5163344
* @e-mail contact          abauer.rafael@gmail.com
*
* @brief   Exercise 6 - Dynamic Circuit Configuration
*
* Description: The main starts by initializing the MSP. The software was adapted from exercise 5.
*              the detection of PB5 and 6 were kept, but two other IOs were declared: COMP_OUT and REL_STATS.
*
*              Two musics arrays are declared, namely We Wish a Merry Christmas and Jingle Bells.
*              The software makes use of two helper classes, Jukebox and MusicSelection to perform the logic of the
*              exercise.
*
*              Since the NC contact is in the DAC_IN, the BUZZER is connected to it, therefore the software starts by expecting an interrupt on COMP_OUT.
*              As soon as a song is selected, the relay is activated and the NO contact (PWM output) is connected to the BUZZER, so a song can be played.
*              When the song finishes to play, the relay is turned off and after 250ms, the interrupt on COMP_OUT is re-enabled.
*
*              The Timer 1 is configured to interrupt every 125 ms. It executes the method to
*              check if there are more notes to be played, calls the MusicSelection to check if the music was selected already, and
*              calls the debouncing of PB5 and PB6.
*
*              There is a Debouncer class (declared in common/Debouncer.hpp) to prevent debouncing, since when the
*              button is pressed once, due to debounce, it can trigger the interruption multiple times. As soon as the button is
*              pressed once, it calls the helper class MusicSelection "saying" that the button has been pressed. This class is
*              responsible to select the song based on how many times the button was pressed within an specific time period.
*
* Pin connections:  BUZZER <-> CON4 middle
*                   DAC_IN <-> CON4 right
*                   CON3:P3.6 <-> CON4 left
*                   REL_STAT <-> CON3:P1.0
*                   PB5 <-> CON3:P1.3
*                   PB6 <-> CON3:P1.4
*                   COMP_OUT <-> CON3:P1.5
*                   Potentiometer in the middle
*                   JP5 to VFO
*
* Theory answers: Answered in theory.txt
*
* Tasks completed:
*  Task 1
*    a)
*         [x] Modify exercise5                       (x/6,0 pt.)
*    b)
*         [x] Two advantages compared to exercise 5  (x/1,0 pt.)
*         [x] Describe signal flow                   (x/2,0 pt.)
*  Task 2
*         [x] feedback.txt                           (x/1.0 pt.)
*
* @note    The project was exported using CCS 12.1.0.00007
******************************************************************************/
#include <templateEMP.h>

#include "Debouncer.hpp"
#include "GPIOs.hpp"
#include "Timer.hpp"

#include "Pwm.hpp"

#include <chrono>

using namespace Microtech;
Pwm pwm(
 GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(6)>());  // Create handle of PWM for pin 6 from port 5

constexpr InputHandle PB5 = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(3)>();
constexpr InputHandle PB6 = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(4)>();

// Handle of pun from same PWM output, but to allow
// to read the piezo when not playing a music.
constexpr InputHandle COMP_OUT = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(5)>();

constexpr OutputHandle REL_STAT = GPIOs::getOutputHandle<IOPort::PORT_1, static_cast<uint8_t>(0)>();

typedef void (Pwm::*PwmFuncPointer)();  // Definition of the a function pointer to a method of the class Pwm.

// Creation of the different notes.
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

// Helper struct used to hold the note itself and its tempo
struct NoteWithTempo {
 PwmFuncPointer note;  // Pointer to the note.
 uint8_t numTicks;     // Tempo of that note.
};

// Struct that holds a music size together with the pointer to the first note.
struct Playlist {
 const size_t musicSize;
 const NoteWithTempo* firstNote;
};

// Declaration of WeWishYouAMerryChristmas
constexpr NoteWithTempo WeWishYouAMerryChristmas[]{
 {C4, 2}, {F4, 2}, {F4, 1}, {G4, 1}, {F4, 1}, {E4, 1}, {D4, 2}, {D4, 2}, {D4, 2},  {G4, 2}, {G4, 1},
 {A4, 1}, {G4, 1}, {F4, 1}, {E4, 2}, {C4, 2}, {C4, 2}, {A4, 2}, {A4, 1}, {Bb4, 1}, {A4, 1}, {G4, 1},
 {F4, 2}, {D4, 2}, {C4, 1}, {C4, 1}, {D4, 2}, {G4, 2}, {E4, 2}, {F4, 4}};

// Declaration of Jingle Bells
constexpr NoteWithTempo JingleBells[]{{A4, 2},  {A4, 2},  {A4, 4},  {A4, 2}, {A4, 2},   {A4, 4},  {A4, 2},
                                     {C5, 2},  {F4, 3},  {G4, 1},  {A4, 8}, {NONE, 2}, {Bb4, 2}, {Bb4, 2},
                                     {Bb4, 3}, {Bb4, 1}, {Bb4, 2}, {A4, 2}, {A4, 2},   {A4, 1},  {A4, 1},
                                     {C5, 2},  {C5, 2},  {Bb4, 2}, {G4, 2}, {F4, 8}};

/**
* Helper class responsible for holding the playlist and to execute the songs when requested.
*/
class Jukebox {
public:
 Jukebox() = default;
 ~Jukebox() = default;

 /**
  * This method is responsible for keeping track of the current note being played and to changing the note
  * when the tempo of the current note has exceeded.
  *
  * @returns If there are still more notes to be played
  */
 bool evaluatePlay() {
   if (!playing) {
     return false;  // return false since we are not playing
   }

   if (currentNoteTempoCounter == 0) {     // The note has already played the correct amount of time
     currentNoteIndex++;                   // Increase the note
     currentNote++;                        // Gets the next pointer.
     if (currentNoteIndex >= musicSize) {  // If we are at the end of the song.
       playing = false;                    // Set playing to false
       songBeingPlayed = false;            // We finished the song
       stop();
       return false;  // Return false since there are no more notes to play
     }
     // If it is not the end of the song
     currentNoteTempoCounter = currentNote->numTicks;  // Updates the tempo counter according to the current note
     (pwm.*(currentNote->note))();                     // Plays the note.
   } else {                                            // If the note has not played the correct amount of time:
     currentNoteTempoCounter--;                        // Decrease note tempo counter.
   }

   return true;  // return true since there are more iterations to be made.
 }

 /**
  * Function used to pause or resume a song.
  */
 void pauseOrResume() {
   // If there is a song being played, we can resume the song, otherwise not.
   if (songBeingPlayed) {
     if (playing) {  // If it is playing we stop
       stop();
     } else {
       startPlaying();
     }
     playing ^= true;  // Toggle playing
   }
 }

 /**
  * Method that requests the Jukebox to play a song
  */
 bool playSong(const uint8_t songNumber) {
   if (songNumber >= PLAYLIST_SIZE) {
     return false;
   }
   currentNoteIndex = 0;
   currentNote = playlist[songNumber].firstNote;
   musicSize = playlist[songNumber].musicSize;
   currentNoteTempoCounter = currentNote->numTicks;
   playing = true;
   songBeingPlayed = true;
   startPlaying();
   return true;
 }

 // Method used to know if the evaluation of the pin used for the piezo input can already be
 // read. It can only be read if there is no song playing and the a certain time has already
 // passed after it stopped playing.
 bool getAllowPiezoEvaluation() const {
   return !playing;
 }

 // Method to get the playlist size
 constexpr size_t getPlaylistSize() const noexcept {
   return PLAYLIST_SIZE;
 }

private:
 /**
  * Method used when the song needs to stop playing.
  * It stops the pwm, enables the PB5 interrupt, and makes the
  * piezo input ready to read the pin.
  */
 void stop() {
   pwm.stop();
   REL_STAT.setState(IOState::LOW);
   PB5.enableInterrupt();
 }

 void startPlaying() {
   REL_STAT.setState(IOState::HIGH);
   COMP_OUT.disableInterrupt();
   PB5.disableInterrupt();                // Disable the PB5 interrupt
   (pwm.*(currentNote->note))();          // Play the current

 }

 // The playlist declaration and initialization with the two songs
 static constexpr Playlist playlist[]{
   {sizeof(WeWishYouAMerryChristmas) / sizeof(WeWishYouAMerryChristmas[0]), &WeWishYouAMerryChristmas[0]},
   {sizeof(JingleBells) / sizeof(JingleBells[0]), &JingleBells[0]},
 };

 const size_t PLAYLIST_SIZE = sizeof(playlist) / sizeof(playlist[0]);  ///< Size of the playlist.
 bool playing = false;                                                 ///< If there are notes being actively played
 bool songBeingPlayed =
   false;  ///< If there is a song being played. Useful to know when we are in the middle of a song and paused.
 uint8_t currentNoteIndex = 0;  ///< Attribute holds the index of the current note being played
 const NoteWithTempo* currentNote = 0;  ///< The pointer pointing to the current note.
 uint8_t currentNoteTempoCounter = 0;   ///< The counter to know the tempo of the current note.
 size_t musicSize = 0;                  ///< The music size of the current note being played
};

// Because it is a static member it also needs to be declared outside of the class declaration
// So the compiler allocates memory for it. Otherwise there will be a linker error.
constexpr Playlist Jukebox::playlist[];

/**
* Classed used to verify which music was selected.
*/
class MusicSelection {
public:
 MusicSelection() = delete;
 /**
  * Constructor of the class. It takes the reference of the Jukebox it is controlling.
  */
 explicit MusicSelection(Jukebox& controllJukebox) : jukebox(controllJukebox) {}

 /**
  * Method called by the timer.
  * It evaluates the number of times a button has been pushed as well as
  * after being pushed, it waits for a certain number of iterations to make
  * define how many times the button was pressed.
  *
  * With the current setup, it waits it checks the number of pushes within one second.
  */
 void evaluateNumberPushed() {
   static uint8_t numberOfEvaluations = 0;
   // Verify if it is supposed to cound pushes
   if (!countPushes) {
     return;
   }

   // If the number of iterations is bigger or equal the one of the necessary to
   // reach one second
   if (numberOfEvaluations >= TIMER_TICKS_FOR_1_SECOND) {
     numberOfEvaluations = 0;  // Resets the number of evaluations.
     uint8_t songNumber = 0;
     if (numberOfPushed >= jukebox.getPlaylistSize()) {  // If the number of pushes is equal or bigger to 2
       songNumber = jukebox.getPlaylistSize() - 1;       // Choose song 2 (1 in the array)
     } else {                    // Otherwise
       songNumber = 0;           // Choose song 1 (0 in the array)
     }
     countPushes = false;            // Stop counting the number of pushes
     jukebox.playSong(songNumber);   // requests jukebox to play the chosen song
   } else {                          // Otherwise
     numberOfEvaluations++;          // Increase the number of evaluations counter
     if (wasButtonPushed) {  // If the button was pushed
       numberOfPushed++;             // Increase number of pushes counter
       wasButtonPushed = false;      // Set the button was pushed flag to false
     }
   }
 }

 /**
  * Method used to make the connection between the pin interrupt and the timer interrupt
  * Whenever the pin gets pushed, this function gets called and sets some internal flags.
  * These flags are then used in the evaluateNumberPushed which is called by the timer.
  */
 void buttonPushed() {
   if (!countPushes) {  // if the counts are not being done,
     numberOfPushed = 0;        // Resets counter,
     countPushes = true;        // and set flag to count pushed to true
   }
   wasButtonPushed = true;
 }

private:
 static constexpr uint8_t TIMER_TICKS_FOR_1_SECOND =
   8;  ///< Constant specifing the number of ticks to count to 1 second.

 bool countPushes = false;  ///< Flag used to know whether the class has to count the number of pushes of the button
 bool wasButtonPushed = false;  ///< Flag to know if the button was pushed
 uint8_t numberOfPushed = 0;    ///< Counter holding the number of times the button was pushed within a time period
 Jukebox& jukebox;              ///< Reference to the jukebox
};

Jukebox jukebox;  ///< Declaration of the jukebox
MusicSelection musicSelection(jukebox);  ///< Declaration of the MusicSelection
Debouncer<MusicSelection> pb5Debouncer(musicSelection,&MusicSelection::buttonPushed);
Debouncer<Jukebox> pb6Debouncer(jukebox, &Jukebox::pauseOrResume);

void playTaskFunc() {
 static uint8_t waitingTimeAfterFinished = 0;
 musicSelection.evaluateNumberPushed();

 if(!(jukebox.evaluatePlay())) {
     if(waitingTimeAfterFinished >= 2) {
         COMP_OUT.enableInterrupt();
     } else {
         waitingTimeAfterFinished++;
     }
 } else {
     waitingTimeAfterFinished = 0;
 }
 pb5Debouncer.evaluateDebounce();
 pb6Debouncer.evaluateDebounce();
}

int main() {
 initMSP();
 constexpr TimerConfigBase<8> TIMER_CONFIG;  // helper class where states whats the CLK_DIV of the clock.
 Timer<1>::getTimer().init(TIMER_CONFIG);

 pwm.init();
 pwm.setDutyCycle(500);

 COMP_OUT.init();
 COMP_OUT.enablePinResistor(IOResistor::PULL_DOWN);
 REL_STAT.init();
 REL_STAT.setState(IOState::LOW);
 PB5.init();
 PB5.enablePinResistor(IOResistor::PULL_UP);
 PB6.init();
 PB6.enablePinResistor(IOResistor::PULL_UP);

 PB5.enableInterrupt();
 PB6.enableInterrupt();
 COMP_OUT.enableInterrupt();

 // Creates a 125ms periodic task for perform the exercise logic.
 TaskHandler<125, std::chrono::milliseconds> playTask(&playTaskFunc, true);

 // registers play task to timer 1
 Timer<1>::getTimer().registerTask(TIMER_CONFIG, playTask);

 // globally enables the interrupts.
 __enable_interrupt();

 while (true) {
   _no_operation();
 }
 return 0;
}

// Port 1 interrupt vector
#pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR(void) {
 // Get bits 3, 4 and 5 => 0011 1000 = 0x38
 const uint8_t pendingInterrupt = getRegisterBits(P1IFG, static_cast<uint8_t>(0x38), static_cast<uint8_t>(3));

 if (pendingInterrupt > 1 && pendingInterrupt < 4) {
   pb6Debouncer.buttonWasPushed();
 } else {
   pb5Debouncer.buttonWasPushed();
 }

 const uint8_t clearFlag = (pendingInterrupt << 0x03);
 resetRegisterBits(P1IFG, clearFlag);  // clear interrupt flag
}
