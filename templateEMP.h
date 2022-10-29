/*
 * File:         templateEMP.h
 *
 * Version:      0.7
 *
 * Authors:      Sebastian Sester & Marc Schink
 *               Laboratory for Electrical Instrumentation
 *               University of Freiburg
 *
 * Creation:     February 2012
 * Last Changes: June 2015
 *
 * Description:  This is the template for the practical course(s) of
 *               "Mikrocomputertechnik".
 *
 * How to use:   Copy this file into:
 *               <path to code composer installation>\ccs_base\msp430\include\
 *
 *               This is most likely:
 *               C:\Program Files\TI\ccsv6\ccs_base\msp430\include
 *
 *               Include it in your project with:
 *               #include <templateEMP.h>
 *
 *               And initialize it with:
 *               initMSP();
 *
 * Please note:
 *   CodeComposer doesn't feature an easy solution on how to include
 *   libraries (you either have to copy the library into every new project
 *   or have to adjust the linker each time - both not too convenient
 *   for someone who is completely new to programming microcontrollers)
 *   As a simply solution to this we decided to put the code in this headerfile
 *   instead. This is a "not so good practice", but it works.
 *   Students, please note: only put declarations into headerfiles, not code.
 *   (Ironic, isn't it?)
 *
 * Also note that you do not have to understand this file in detail but
 *  should rather understand its comments so that you know what a certain
 *  function does - instead of how.
 *
 * If you want to implement UART on your own, please write
 *   #define NO_TEMPLATE_UART 1
 * right before you include this file.
 *
 * If you want to implement the ISR on your own, but still want to keep the
 * other functions, please write
 *   #define NO_TEMPLATE_ISR 1
 * right before you include this file.
 *
 *
 * Changelog:
 *   0.1: Creation
 *   0.2: Fixed the ring buffer not going in a ring (buffer overflows, yay!).
 *   0.3: Added ifndef-structure to allow users to disable uart completely.
 *        Define NO_TEMPLATE_UART in case you want to write your own
 *        implementation.
 *   0.4: Fixed serialPrintInt as reported by Nessim Ben Ammar.
 *   0.5: General clean up of the code and corrected some comments.
 *   0.6: Fixed some comments and added some more
 *   0.7: It's now possible to use NO_TEMPLATE_ISR to disable the ISR-code
 *        while still maintaining the other serial functions (in case you have/
 *        want to implement your own ISR).
 */

#ifndef TEMPLATEEMP_H_
  #define TEMPLATEEMP_H_

  #include <msp430g2553.h>

  #ifndef NO_TEMPLATE_UART
    // We use a ringbuffer for receiving data. RXBUFFERSIZE defines its size.
    // If you change this: make sure you read the data fast enough (if you
    // lower it) or that you really have a lot of free space (if you increase
    // it)
    #define RXBUFFERSIZE 32

    // These variables contain the serial ringbuffer as well as the current
    // positions within that buffer.
    char volatile rxBuffer[RXBUFFERSIZE];
    char volatile rxBufferStart = 0;
    char volatile rxBufferEnd = 0;
    // The error flag for the serial buffer (e.g. if an overflow happened)
    char rxBufferError = 0;
    // Echo flag. This is 1 if received text should be printed, too.
    // (This allows the users to see what they just entered.)
    char echoBack = 0;

    /**
     * serialEchoBack
     * This determines if the user's input should be echoed back or not.
     * 
     * @param e   0 if no echo is required, anything else if it is.
     */
    void serialEchoBack(char e) {
      // This is the ternary operator. If e is 0, echoBack will be set 0,
      // else it will be set to 1 (we only need this because you can pass
      // other values but 0 and 1 to the function).
      echoBack = e?1:0;
    }

    /**
     * This function can be used to check for an buffer-error such as a buffer
     * overflow. Calling this function will also reset the error-variable.
     *
     * @return 0 if there is no error, anything elese if there is one.
     */
    char serialError() {
      char r = rxBufferError;
      rxBufferError = 0;
      return r;
    }

    /**
     * Echo one character to the serial connection. Please note that this
     * function will not work with UTF-8-characters so you should stick
     * to ANSI or ASCII.
     *
     * @param char The character to be displayed.
     */
    void serialWrite(const char tx) {
      // Loop until the TX buffer is ready.
      while (!(IFG2&UCA0TXIFG));
      // Write the character into the TX-register.
      UCA0TXBUF = tx;
      // And wait until it has been transmitted.
      while (!(IFG2&UCA0TXIFG));
    }

    /**
     * Print a given integer as a readable number to serial connection (using
     * the ASCII charmap).
     *
     * @param i   The number to be displayed; 16 bit max.
     */
    void serialPrintInt(int i) {
      int j = i;
      // If the number is between 10000 and 65535, print the 10000-
      // digit.
      if (j >= 10000) {
        serialWrite(0x30 + i/10000);
      }
      // Remove the 10000-digit.
      i = i % 10000;
      // Print the 1000-digit, if the number bigger then 999.
      if (j >= 1000) {
        serialWrite(0x30 + i/1000);
      }
      // Now remove the 1000-digit.
      i = i % 1000;
      // Print the 100-digit if the number is big enough ...
      if (j >= 100) {
        serialWrite(0x30 + i/100);
      }
      // ... remove it ...
      i = i % 100;
      // ... same for 10-digit ...
      if (j >= 10) {
        serialWrite(0x30 + i/10);
      }
      // ...
      i = i % 10;
      // Print the last digit, no matter how big the number is (so if the
      // number is 0, we'll just print that).
      serialWrite(0x30 + i/1);
    }

    /**
     * Print a sequence of characters to the serial connection.
     *
     * @example     serialPrint("output");
     * @param tx    A pointer to the text that shall be printed. Has to be
     *              terminated by \0
     */
    void serialPrint(const char* tx) {
      int b, i = 0;
      // Count the number of bytes we shall display.
      while(tx[i] != 0x00) {
        i++;
      }
      // Write each of the bytes we counted.
      for (b = 0; b < i; b++) {
        // We already implemented the "write-a-single-character"-function,
        // so we're going to use that function instead of implementing the
        // same stuff here again.
        serialWrite(tx[b]);
      }
    }

    /**
     * Print a sequence of characters to the serial connection and terminate
     * the string with a linebreak. (Note that you'll have to enable "Newline
     * at LF+CR" within HTerm - if you use HTerm.)
     *
     * @example     serialPrint("output");
     * @param tx    A pointer to the text that shall be printed. Has to be
     *              terminated by \0
     */
    void serialPrintln(const char* tx) {
      // We don't have to implement this again, just pass tx to the apropriate
      // function.
      serialPrint(tx);
      // Print \n
      serialWrite(0x0D);
      // Print \r
      serialWrite(0x0A);
    }

    /**
     * Returns 1 if the serial buffer is not empty i.e. some data has been
     * received on the serial connection (e.g. by sending something with HTerm)
     *
     * @return 1 if there is data, 0 if not.
     */
    char serialAvailable(void) {
      // If the buffer's start is not the buffer's end, there's data (return 1)
      if (rxBufferStart != rxBufferEnd) {
        return 1;
      }
      // Else there is none (return 0)
      return 0;
    }

    /**
     * Clear the serial buffer; all content will be lost.
     */
    void serialFlush(void) {
      // Set the buffer's start to the buffer's end.
      rxBufferStart = rxBufferEnd;
    }

    /**
     * Returns the first byte from the serial buffer without modifying the
     * same. Returns -1 if the buffer is empty.
     *
     * @return The first byte within the buffer or -1 if the buffer is empty.
     */
    int serialPeek(void) {
      // If the buffer's start is the buffer's end, there's no data (return -1)
      if (rxBufferStart == rxBufferEnd) {
        return -1;
      }
      // Return the first byte
      return rxBuffer[rxBufferStart];
    }

    /**
     * Returns the first byte from the serial buffer and removes it from the
     * same. Returns -1 if the buffer is empty.
     *
     * @return The first byte within the buffer or -1 if the buffer is empty.
     */
    int serialRead(void) {
      // If the buffer's start is the buffer's end, there's no data (return -1)
      if (rxBufferStart == rxBufferEnd) {
        return -1;
      }
      // Save the first byte to a temporary variable, move the start-pointer
      char r = rxBuffer[rxBufferStart++];
      rxBufferStart %= RXBUFFERSIZE;
      // and return the stored byte.
      return r;
    }

    /**
     * Reads in a number from the serial interface, terminated by any
     * non-numeric character.
     *
     * WARNING: This is a *very basic* implementation and you might want to
     * write your own depending on your scenario and your needs.
     *
     * @return The read-in-number.
     */
    int serialReadInt(void) {
      int number = 0;
      char stop = 0;
      char negative = 0;
      // While we didn't meet any non-numeric character
      while (!stop) {
        // Wait for data
        while (!serialAvailable());
        // Read the character
        char letter = serialRead();
        // If it's a minus and this is the first figure, it's a negative number
        if (letter == '-' && number == 0) {
          negative = 1;
        }
        // If it's a number, add the it to the resulting number
        else if (letter >= '0' && letter <= '9') {
          number = number * 10 + (letter - '0');
        }
        // Stop the interpretation elsewise.
        else {
          stop = 1;
        }
      }
      if (negative) {
        return number * -1;
      }
      return number;
    }

    /**
     * The UART interrupt (aka. "Hey, we received something!")
     * You must not call this function directly, it's invoked by the controller
     * whenever some data is received on the serial connection.
     */
    #ifndef NO_TEMPLATE_ISR
      #pragma vector=USCIAB0RX_VECTOR
      __interrupt void USCI0RX_ISR(void) {
        // Store the received byte in the serial buffer. Since we're using a
        // ringbuffer, we have to make sure that we only use RXBUFFERSIZE bytes.
        rxBuffer[rxBufferEnd++] = UCA0RXBUF;
        rxBufferEnd %= RXBUFFERSIZE;
        // If enabled, print the received data back to user.
        if (echoBack) {
          while (!(IFG2&UCA0TXIFG));
          UCA0TXBUF = UCA0RXBUF;
        }
        // Check for an overflow and set the corresponding variable.
        if (rxBufferStart == rxBufferEnd) {
          rxBufferError = 1;
        }
      }
    #endif  /*NO_TEMPLATE_ISR*/
  #endif  /*NO_TEMPLATE_UART*/

  /**
   * Initialization of the controller
   * This function sets the clock, stops the watchdog and - if allowed -
   * initialize the USART-machine.
   */
  void initMSP(void) {
    // Stop Watchdog Timer
    WDTCTL = WDTPW + WDTHOLD;
    // If the calibration constants were erased, stop here.
    if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF) {
      while(1);
    }

    // Set clock to 1 MHz. Please don't change this if you have to upload/share
    // your code during the lab.
    // Possible options: _1 _8 _12 _16. Don't forget to adapt UART if you
    // change this!
    BCSCTL1 = CALBC1_1MHZ;
    // Set DCO step + modulation
    DCOCTL  = CALDCO_1MHZ;

    #ifndef NO_TEMPLATE_UART
      // Activate UART on 1.1 / 1.2
      // (fixed connection to PC, shows up there as COMx)
      P1SEL  = BIT1 + BIT2;           // P1.1 = RXD, P1.2=TXD, set everything
      P1SEL2 = BIT1 + BIT2;           // else as a normal GPIO.
      UCA0CTL1 |= UCSSEL_2;           // Use the SMCLK
      UCA0BR0 = 104;                  // 9600 Baud at 1 MHz
      UCA0BR1 = 0;                    // 9600 Baud at 1 MHz
      UCA0MCTL = UCBRS0;              // Modulation UCBRSx = 1
      UCA0CTL1 &= ~UCSWRST;           // Initialize USCI state machine
      IE2 |= UCA0RXIE;                // Enable USCI_A0 RX interrupt
    #endif  /*NO_TEMPLATE_UART*/

    // Now enable the global interrupts
    __enable_interrupt();

    #ifndef NO_TEMPLATE_UART
      // Boot-up message
      serialWrite(0x0C);
      serialPrintln("Launchpad booted.");
    #endif  /*NO_TEMPLATE_UART*/
  }

#endif /*TEMPLATEEMP_H_*/
