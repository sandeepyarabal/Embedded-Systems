/**********************************************************************
 *
 * Filename:    main.c
 *
 * Description: The main program file the serial driver example.
 *
 * Notes:       This file is specific to the Arcom board.
 *
 * Copyright (c) 2006 Anthony Massa and Michael Barr. All rights reserved.
 * This code is from the book Programming Embedded Systems, With C and
 * GNU Development Tools, 2nd Edition.
 * It is provided AS-IS, WITHOUT ANY WARRANTY either expressed or implied.
 * You may study, use, and modify it for any non-commercial purpose,
 * including teaching and use in open-source projects.
 * You may distribute it non-commercially as long as you retain this notice.
 * For a commercial use license, or to purchase the book,
 * please visit http://www.oreilly.com/catalog/embsys2.
 *
 **********************************************************************/

#include "stdint.h"
#include "serial.h"
#include "pxa255.h"
#include "viperlite.h"



 /**********************************************************************
  *
  * Function:    main
  *
  * Description: Exercise the serial device driver.
  *
  * Notes:
  *
  * Returns:     This routine contains an infinite loop, which can
  *              be exited by entering q.
  *
  **********************************************************************/
int main(void)
{
    char rcvChar = 0;

    /* Configure the UART for the serial driver. */
    serialInit();

    serialPutChar('s');
    serialPutChar('t');
    serialPutChar('a');
    serialPutChar('r');
    serialPutChar('t');
    serialPutChar('\r');
    serialPutChar('\n');

    while (rcvChar != 'q')
    {
        /* Wait for an incoming character. */
        rcvChar = serialGetChar();

        /* Echo the character back along with a carriage return and line feed. */
        serialPutChar(rcvChar);
        serialPutChar('\r');
        serialPutChar('\n');
    }

    return 0;
}

//serial.c file
#define UART_BAUD_RATE_DIVISOR(baud)    ((14745600/(16*(baud))))
#define TRANSMITTER_EMPTY               (0x40)
#define DATA_READY                      (0x01)


typedef enum {
    PARITY_NONE, PARITY_ODD = PARITY_ENABLE,
    PARITY_EVEN = (PARITY_ENABLE | EVEN_PARITY_ENABLE)
} parity_t;

typedef enum {
    DATA_5, DATA_6 = DATABITS_LENGTH_0, DATA_7 = DATABITS_LENGTH_1,
    DATA_8 = (DATABITS_LENGTH_0 | DATABITS_LENGTH_1)
} databits_t;

typedef enum { STOP_1, STOP_2 = STOP_BITS } stopbits_t;

typedef struct
{
    uint32_t dataBits;
    uint32_t stopBits;
    uint32_t baudRate;
    parity_t parity;
} serialparams_t;

typedef struct
{
    uint32_t data;
    uint32_t interruptEnable;
    uint32_t interruptStatus;
    uint32_t uartConfig;
    uint32_t pinConfig;
    uint32_t uartStatus;
    uint32_t pinStatus;
} volatile uart_t;

serialparams_t gSerialParams;
uart_t* pSerialPort = (uart_t*)(0x40100000);


/**********************************************************************
 *
 * Function:    serialConfig
 *
 * Description: Set the UART communication parameters.
 *
 * Notes:       This function is specific to the Arcom board.
 *
 * Returns:     None.
 *
 **********************************************************************/
static void serialConfig(serialparams_t* params)
{
    uint32_t baudRateDivisor;

    /* Disable the UART before setting the communication parameters. */
    pSerialPort->interruptEnable &= ~(UART_PORT_ENABLE);

    /* Set the communication parameters in the UART. */
    pSerialPort->uartConfig = (params->dataBits | params->parity | params->stopBits);

    /* Configure the baud rate. */
    /* IMPORTANT NOTE: When the divisor access bit (DLAB) is set in the UART config
     * register (LCR), reads and writes to offset 0 and offset 4 access the
     * DLL and DLH registers, respectively. */
    pSerialPort->uartConfig |= DIVISOR_ACCESS_ENABLE;
    baudRateDivisor = (UART_BAUD_RATE_DIVISOR(params->baudRate));

    pSerialPort->interruptEnable = ((baudRateDivisor >> 8) & 0xFF);
    pSerialPort->data = (baudRateDivisor & 0xFF);

    /* Enable access to the UART data registers. */
    pSerialPort->uartConfig &= ~(DIVISOR_ACCESS_ENABLE);

    /* Reenable the UART now that the parameters are set. */
    pSerialPort->interruptEnable |= UART_PORT_ENABLE;
}


/**********************************************************************
 *
 * Function:    serialInit
 *
 * Description: Initialize the serial port UART.
 *
 * Notes:       This function is specific to the Arcom board.
 *              Default communication parameters are set in
 *              this function.
 *
 * Returns:     None.
 *
 **********************************************************************/
void serialInit(void)
{
    static int bInitialized = FALSE;

    /* Initialize the UART only once. */
    if (bInitialized == FALSE)
    {
        /* Set the communication parameters. */
        gSerialParams.baudRate = 115200;
        gSerialParams.dataBits = DATA_8;
        gSerialParams.parity = PARITY_NONE;
        gSerialParams.stopBits = STOP_1;

        serialConfig(&gSerialParams);

        bInitialized = TRUE;
    }
}


/**********************************************************************
 *
 * Function:    serialPutChar
 *
 * Description: Send a character via the serial port.
 *
 * Notes:       This function is specific to the Arcom board.
 *
 * Returns:     None.
 *
 **********************************************************************/
void serialPutChar(char outputChar)
{
    /* Wait until the transmitter is ready for the next character. */
    while ((pSerialPort->uartStatus & TRANSMITTER_EMPTY) == 0)
        ;

    /* Send the character via the serial port. */
    pSerialPort->data = outputChar;
}


/**********************************************************************
 *
 * Function:    serialGetChar
 *
 * Description: Get a character from the serial port.
 *
 * Notes:       This function is specific to the Arcom board.
 *
 * Returns:     The character received from the serial port.
 *
 **********************************************************************/
char serialGetChar(void)
{
    /* Wait for the next character to arrive. */
    while ((pSerialPort->uartStatus & DATA_READY) == 0)
        ;

    return pSerialPort->data;
}
