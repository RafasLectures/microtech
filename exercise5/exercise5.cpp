/*****************************************************************************
 * @file                    exercise5.cpp
 * @author                  Rafael Andrioli Bauer
 * @date                    14.12.2022
 * @matriculation number    5163344
 * @e-mail contact          abauer.rafael@gmail.com
 *
 * @brief   Exercise 5 - Pulse Width Modulation
 *
 * Description: The main starts by initializing the MSP. Followed by the timer 1. The PB5 and 6 can be enabled by
 *              declaring a macro (#define) named CAPTURE_FROM_PB5. When that is done, then the code for detecting
 *              the input interrupt is enabled, as well as the logic of the Timer 1 interruption changes.
 *
 *              Two musics arrays are declared, namely We Wish a Merry Christmas and Jingle Bells.
 *              The software makes use of two helper classes, Jukebox and MusicSelection to perform the logic of the
 *              exercise.
 *
 *              The Timer 1 is configured to interrupt every 125 ms. When we are note reading the buttons, the timer
 *              simply calls the Jukebox to evaluate if it needs to change note. That function will return if there are notes still to
 *              be processed. When there are no more notes to be processed, then the timer interrupt requests to start the next song.
 *
 *              When we expect to read from the buttons, there is a Debounce class to prevent debouncing, since when the
 *              button is pressed once, due to debounce, it can trigger the interruption multiple times. As soon as the button is
 *              pressed once, it calls the helper class MusicSelection "saying" that the button has been pressed. This class is
 *              responsible to select the song based on how many times the button was pressed within an specific time period.
 *
 *              When expecting to read from the buttons, the timer interruption is different. It executes the method to
 *              check if there are more notes to be played, calls the MusicSelection to check if the music was selected already, and
 *              calls the debouncing of PB5 and PB6.
 *
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
 *    a)   [x] PWM with 50% duty-cycle            (x/1,0 pt.)
 *         [x] Store melody in array              (x/1,0 pt.)
 *         [x] Melody 1                           (x/1,0 pt.)
 *         [x] Melody 2                           (x/1,0 pt.)
 *   b)
 *         [x] Capture PB5 with interrupt         (x/1,0 pt.)
 *         [x] Press once play melody one         (x/1,0 pt.)
 *         [x] Press twice play melody two        (x/1,0 pt.)
 *   c)
 *         [x] Detect board knock from piezo P3IN (x/2,0 pt.)
 *   d)
 *         [x] PB6 as a pause/resume button       (x/1,0 pt.)
 *
 *  Task 2
 *         [x] feedback.txt                       (x/1.0 pt.)
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
Pwm pwm(
  GPIOs::getOutputHandle<IOPort::PORT_3, static_cast<uint8_t>(6)>());  // Create handle of PWM for pin 6 from port 5

#ifdef CAPTURE_FROM_PB5  // For task 1.a) please comment this line.

constexpr InputHandle pb5 = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(3)>();
constexpr InputHandle pb6 = GPIOs::getInputHandle<IOPort::PORT_1, static_cast<uint8_t>(4)>();

constexpr InputHandle piezoIn =
  GPIOs::getInputHandle<IOPort::PORT_3, static_cast<uint8_t>(6)>();  // Handle of pun from same PWM output, but to allow
                                                                     // to read the piezo when not playing a music.
#endif

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
  {F4, 2}, {D4, 2}, {C4, 1}, {C4, 1}, {D4, 2}, {G4, 2}, {E4, 2}, {F4, 4}, {NONE, 8}};

// Declaration of Jingle Bells
constexpr NoteWithTempo JingleBells[]{{A4, 2},  {A4, 2},  {A4, 4},  {A4, 2}, {A4, 2},   {A4, 4},  {A4, 2},
                                      {C5, 2},  {F4, 3},  {G4, 1},  {A4, 8}, {NONE, 2}, {Bb4, 2}, {Bb4, 2},
                                      {Bb4, 3}, {Bb4, 1}, {Bb4, 2}, {A4, 2}, {A4, 2},   {A4, 1},  {A4, 1},
                                      {C5, 2},  {C5, 2},  {Bb4, 2}, {G4, 2}, {F4, 8},   {NONE, 8}};

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
    // Counter to give a delay after stopped playing. Usually used to know
    // if the piezo can already be read.
    static uint8_t notPlayingCounter = 0;
    if (playing == false) {
      if (notPlayingCounter < 3) {
        timePassedAfterStoppedPlaying = false;
        notPlayingCounter++;
      } else {
        timePassedAfterStoppedPlaying = true;
      }

      return false;  // return false since we are not playing
    }
    // If it is playing the counter goes to 0.
    notPlayingCounter = 0;
    timePassedAfterStoppedPlaying = false;

    // Check if we are in the first note and it is the first time we are iterating over it
    if (currentNoteIndex == 0 && currentNoteTempoCounter == currentNote->numTicks) {
      (pwm.*(currentNote->note))();  // Play the note
      songBeingPlayed = true;
      currentNoteTempoCounter--;              // Decrease the note tempo counter
    } else {                                  // If it is not the first note
      if (currentNoteTempoCounter == 0) {     // The note has already played the correct amount of time
        currentNoteIndex++;                   // Increase the note
        currentNote++;                        // Gets the next pointer.
        if (currentNoteIndex >= musicSize) {  // If we are at the end of the song.
          playing = false;                    // Set playing to false
          songBeingPlayed = false;            // We finished the song
#ifdef CAPTURE_FROM_PB5
          stop();
#endif
          return false;  // Return false since there are no more notes to play
        }
        // If it is not the end of the song
        currentNoteTempoCounter = currentNote->numTicks;  // Updates the tempo counter according to the current note
        (pwm.*(currentNote->note))();                     // Plays the note.
      } else {                                            // If the note has not played the correct amount of time:
        currentNoteTempoCounter--;                        // Decrease note tempo counter.
      }
    }

    return true;  // return true since there are more iterations to be made.
  }

#ifdef CAPTURE_FROM_PB5
  /**
   * Function used to pause or resume a song.
   */
  void pauseOrResume() {
    // If there is a song being played, we can resume the song, otherwise not.
    if (songBeingPlayed == true) {
      if (playing == true) {  // If it is playing we stop
        stop();
      } else {
        pwm.init();                            // Sets the pin as PWM
        piezoIn.enableResistor(false, false);  // disable the pulldown resistor
        (pwm.*(currentNote->note))();          // Play the current
        pb5.disableInterrupt();                // Disable the PB5 interrupt
      }
      playing ^= true;  // Toggle playing
    }
  }
#endif
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

#ifdef CAPTURE_FROM_PB5
    // Initializes the output to be a PWM and disable the pull-down resistor.
    pwm.init();
    piezoIn.enableResistor(false, false);
#endif
    playing = true;
    return true;
  }

  // Method used to know if the evaluation of the pin used for the piezo input can already be
  // read. It can only be read if there is no song playing and the a certain time has already
  // passed after it stopped playing.
  bool getAllowPiezoEvaluation() {
    return playing == false && timePassedAfterStoppedPlaying == true;
  }

  // Method to get the playlist size
  constexpr size_t getPlaylistSize() const noexcept {
    return PLAYLIST_SIZE;
  }

private:
#ifdef CAPTURE_FROM_PB5
  /**
   * Method used when the song needs to stop playing.
   * It stops the pwm, enables the PB5 interrupt, and makes the
   * piezo input ready to read the pin.
   */
  void stop() {
    pwm.stop();
    pb5.enableInterrupt();
    piezoIn.init();
    piezoIn.enableResistor(true, false);
  }
#endif
  // The playlist declaration and initialization with the two songs
  static constexpr Playlist playlist[]{
    {sizeof(WeWishYouAMerryChristmas) / sizeof(WeWishYouAMerryChristmas[0]), &WeWishYouAMerryChristmas[0]},
    {sizeof(JingleBells) / sizeof(JingleBells[0]), &JingleBells[0]},
  };

  const size_t PLAYLIST_SIZE = sizeof(playlist) / sizeof(playlist[0]);  ///< Size of the playlist.
  bool playing = false;                                                 ///< If there are notes being actively played
  bool songBeingPlayed =
    false;  ///< If there is a song being played. Useful to know when we are in the middle of a song and paused.
  bool timePassedAfterStoppedPlaying =
    false;                       ///< Attribute used to know if there has been a time after the song stopped playing.
  uint8_t currentNoteIndex = 0;  ///< Attribute holds the index of the current note being played
  const NoteWithTempo* currentNote = 0;  ///< The pointer pointing to the current note.
  uint8_t currentNoteTempoCounter = 0;   ///< The counter to know the tempo of the current note.
  size_t musicSize = 0;                  ///< The music size of the current note being played
};

// Because it is a static member it also needs to be declared outside of the class declaration
// So the compiler allocates memory for it. Otherwise there will be a linker error.
constexpr Playlist Jukebox::playlist[];

#ifdef CAPTURE_FROM_PB5
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
    if (countPushes == false) {
      return;
    }

    // If the number of iterations is bigger or equal the one of the necessary to
    // reach one second
    if (numberOfEvaluations >= TIMER_TICKS_FOR_1_SECOND) {
      numberOfEvaluations = 0;  // Resets the number of evaluations.
      uint8_t songNumber = 0;
      if (numberOfPushed >= 2) {  // If the number of pushes is equal or bigger to 2
        songNumber = 1;           // Choose song 2 (1 in the array)
      } else {                    // Otherwise
        songNumber = 0;           // Choose song 1 (0 in the array)
      }
      countPushes = false;            // Stop counting the number of pushes
      jukebox.playSong(songNumber);   // requests jukebox to play the chosen song
      pb5.disableInterrupt();         // Disables the interrupt of PB5.
    } else {                          // Otherwise
      numberOfEvaluations++;          // Increase the number of evaluations counter
      if (wasButtonPushed == true) {  // If the button was pushed
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
    if (countPushes == false) {  // if the counts are not being done,
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
#endif

Jukebox jukebox;  ///< Declaration of the jukebox

#ifdef CAPTURE_FROM_PB5
MusicSelection musicSelection(jukebox);  ///< Declaration of the MusicSelection
/**
 * Classed used to filter the debounce of the input. It can generate multiple interrupts due to debouncing.
 * This can be done with template to be easier, but since I don't have much time, it will stay like this.
 */
class Debouncer {
public:
  typedef void (MusicSelection::*MusicSelectionFuncPointer)();  ///< Definition of type of a function pointer from the
                                                                ///< MusicSelection class
  typedef void (Jukebox::*JukeboxFuncPointer)();  ///< Definition of type of a function pointer from the Jukebox class

  Debouncer() = delete;
  explicit Debouncer(MusicSelectionFuncPointer musicSelectionFunc, JukeboxFuncPointer jukeboxFuncPtr)
    : musicSelectionFunction(musicSelectionFunc), jukeboxFunction(jukeboxFuncPtr) {}

  /**
   * Function called by the timer and make sure that debouncing doesn't happen.
   */
  void evaluateDebounce() {
    if (buttonPushed == true) {  // If the button was pushed
      debounceCounter++;         // Increase debounce counter

      if (debounceCounter > 2) {  // If the counter is bigger than 2
        allowTrigger = true;      // Allows trigger when the button is pushed
        buttonPushed = false;
        debounceCounter = 0;
      }
    }
  }

  /**
   * Method called by the interrupt when the button is pushed
   */
  void buttonWasPushed() {
    buttonPushed = true;
    if (allowTrigger == true) {                        // If the trigger is allowed
      if (musicSelectionFunction != nullptr) {         // Make sure function pointer is not null
        (musicSelection.*(musicSelectionFunction))();  // Calls the function pointer
      }
      // same block as above, but different function pointer.
      if (jukeboxFunction != nullptr) {
        (jukebox.*(jukeboxFunction))();
      }
      allowTrigger = false;  // disallow trigger from the button.
    }
  }

private:
  uint8_t debounceCounter = 0;
  bool allowTrigger = true;
  bool buttonPushed = false;
  MusicSelectionFuncPointer musicSelectionFunction = nullptr;
  JukeboxFuncPointer jukeboxFunction = nullptr;
};

Debouncer pb5Debouncer(&MusicSelection::buttonPushed, nullptr);
Debouncer pb6Debouncer(nullptr, &Jukebox::pauseOrResume);
#endif

void playTaskFunc() {
#ifndef CAPTURE_FROM_PB5
  // Variable that tracks which song is being played.
  static size_t songBeingPlayed = (jukebox.getPlaylistSize() - 1);
  if (jukebox.evaluatePlay() == false) {                       // If there are no more notes to play
    if (songBeingPlayed >= (jukebox.getPlaylistSize() - 1)) {  // and we are at the end of the playlist
      songBeingPlayed = 0;                                     // Start from song 0
    } else {                                                   // otherwise
      songBeingPlayed++;                                       // go to next song
    }
    jukebox.playSong(songBeingPlayed);  // Request jukebox to play song selected above.
  }
#else
  musicSelection.evaluateNumberPushed();
  jukebox.evaluatePlay();
  pb5Debouncer.evaluateDebounce();
  pb6Debouncer.evaluateDebounce();

#endif
}

int main() {
  initMSP();
  constexpr TimerConfigBase<8> TIMER_CONFIG;  // helper class where states whats the CLK_DIV of the clock.
  Timer<1>::getTimer().init(TIMER_CONFIG);

  pwm.init();
  pwm.setDutyCycle(500);

#ifdef CAPTURE_FROM_PB5
  piezoIn.init();
  piezoIn.enableResistor(true, false);
  pb5.init();
  pb6.init();

  pb5.enableInterrupt();
  pb6.enableInterrupt();
#endif

  // Creates a 125ms periodic task for perform the exercise logic.
  TaskHandler<125, std::chrono::milliseconds> playTask(&playTaskFunc, true);

  // registers play task to timer 1
  Timer<1>::getTimer().registerTask(TIMER_CONFIG, playTask);

  // globally enables the interrupts.
  __enable_interrupt();

  while (true) {
#ifdef CAPTURE_FROM_PB5
    if (jukebox.getAllowPiezoEvaluation() == true) {
      if (piezoIn.getState() == IOState::HIGH) {
        pb5Debouncer.buttonWasPushed();
        // musicSelection.buttonPushed();
      }
    }
#else
    _no_operation();
#endif
  }
  return 0;
}

#ifdef CAPTURE_FROM_PB5
// Port 1 interrupt vector
#pragma vector = PORT1_VECTOR
__interrupt void Port_1_ISR(void) {
  // Get bits 3 and 4 => 0001 1000 = 0x18
  const uint8_t pendingInterrupt = getRegisterBits(P1IFG, static_cast<uint8_t>(0x18), static_cast<uint8_t>(3));

  if (pendingInterrupt > 1) {
    pb6Debouncer.buttonWasPushed();
  } else {
    pb5Debouncer.buttonWasPushed();
  }

  const uint8_t clearFlag = (pendingInterrupt << 0x03);
  resetRegisterBits(P1IFG, clearFlag);  // clear interrupt flag
}
#endif
