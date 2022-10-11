#include "hardware/regs/addressmap.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/timer.h"
#include "hardware/regs/m0plus.h"

.syntax unified                                                 @ Specify unified assembly syntax
.cpu    cortex-m0plus                                           @ Specify CPU type is Cortex M0+
.thumb                                                          @ Specify thumb assembly for RP2040
.global main_asm                                                @ Provide program starting address to the linker
.align 4                                                        @ Specify code alignment

.equ    GPIO_BTN_INPT, 21              @ Specify pin for the input from player
.equ    ALRM_ISR_OFFSET, 0x40          @ Vector table entry 16 for ALARM Interrupt
.equ    GPIO_ISR_OFFSET, 0x74          @ Vector table entry 29 for GPIO Interrupt
.equ    GPIO_BTN_DN_MSK, 0x00400000    @ Bit-22 for detecting falling-edge event on GP21
.equ    GPIO_BTN_UP_MSK, 0x00800000    @ Bit-22 for detecting rising-edge event on GP21
.equ    INPUT_END_TIME, 2000000        @ Specify the default alarm timeout - 2seconds
.equ    INITIAL_STATE, 0               @ Initially Alarm has not been run


main_asm:                               @ Entry point to the ASM portion of the program
    push     {lr}                       @ Stores the last state from c code so it knows where to return
    bl       init_pin                   @ Intialises pins (from previous labs)
    bl       install_alrm_isr           @ Installs alarm (from previous labs)
    bl       install_gpio_isr           @ Installs gpio (from previous labs)
    ldr      r0, =wlcm_msg              @ Stores message in r0
    bl       printf                     @ And print using c function
    ldr      r0, =inpt_msg
    bl       printf
    wfi                                 @ No need to set alarm for the First Input

loop:
    bl      set_alarm                   @ Loop to reset the alarm for 2 seconds after each input
    wfi                                 @ Wait for interrupt @ changed later
    ldr     r2, =atimer                 @ Variable will check if alarm interrupt has run or not
    ldr     r1, [r2]                    @ Loading the variable value into r1
    movs    r0, #1                      @ Put constant 1 in r0
    cmp     r0, r1                      @ Comparison of variable atimer
    bne     loop                        @ Loop if not same (wait until equal)
    movs    r0, #0                      @ Store 0 to reset ALARM Interrupt state for next input
    str     r0, [r2]                    @ Update the value by storing it back in atimer variable
    movs    r0, #3                      @ In add_input function in c, code for end of string is 3
    movs    r1, #0                      @ Sets the next character in this case '\0' to be inputted at next index
    bl      add_input                   @ Call the c function to add the input
    pop     {pc}                        @ Return back to the next line of code in c

init_pin:
    push    {lr}                        @ Store the link register to the stack as we will call nested subroutines
    movs    r0, #GPIO_BTN_INPT          @ This value is the GPIO 21 pin on the PI PICO board
    bl      asm_gpio_init               @ Call the subroutine to initialise the GPIO pin specified by r0
    movs    r0, #GPIO_BTN_INPT          @ This value is the GPIO 21 pin on the PI PICO board
    bl      asm_gpio_set_irq            @ Call the subroutine to set up falling edge and rising edge on the GPIO pin specified by r0
    pop     {pc}                        @ Pop the link register from the stack to the program counter

set_alarm:
    ldr     r2,=TIMER_BASE                   @ Get the TIMER_BASE register address
    movs    r1, #1                           @ 1 is the appropriate value to set the alarm
    str     r1, [r2, #TIMER_INTE_OFFSET]     @ Enable the timer 0 interrupt
    ldr     r1, [r2, #TIMER_TIMELR_OFFSET]   @ Get the lower Time registers
    ldr     r3, =ltimer                      @ Get the current flashing interval address
    ldr     r0, [r3]                         @ Get the current flashing interval
    add     r1, r0                           @ Update current alarm interval value
    str     r1, [r2, #TIMER_ALARM0_OFFSET]   @ Set the new alarm
    bx      lr                               @ Branch and exchange with the last instruction in link register

install_alrm_isr:
    ldr     r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)       @ Adds the base address of the CPU in RP2040 to vector offset address to find out where vector table is
    ldr     r1, [r2]                                   @ Hexadecimal address of where the vector table(value stored in r2) should be loaded in r1. r1 now has actual vector table base address
    movs    r2, #ALRM_ISR_OFFSET                       @ Move offset address to r2
    add     r2, r1                                     @ Adds offset address to the base address, stores the whole value in r2. r2 now has the address to the service routine
    ldr     r0, =alrm_isr                              @ Address of the subroutine alrm_isr is stored in r0
    str     r0, [r2]                                   @ Installs the subroutine where we have the offset address of the table
    movs    r0, #1                                     @ Moves the appropriate value into r0
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)  @ Adds the base address of the CPU in RP2040 to NVIC ICPR offset address
    str     r0, [r1]                                   @ Disable the ALARM0 IRQ by writing #1 to the procured address
    ldr     r1, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)  @ Adds the base address of the CPU in RP2040 to NVIC ISER offset address
    str     r0, [r1]                                   @ Enable the ALARM0 IRQ by writing #1 to the procured address
    bx      lr                                         @ Branch and exchange with the last instruction in link register

.thumb_func                     @ Required for all interrupt service routines

alrm_isr:
    push    {lr}                          @ Store the link register to the stack as we will call nested subroutines
    ldr     r2, =TIMER_BASE               @ Get the Timer_Base register address
    movs    r1, #1                        @ 1 is the appropriate value to set the alarm
    str     r1, [r2, #TIMER_INTR_OFFSET]  @ Reset the alarm 
    ldr     r2, =atimer                   @ Retrieve the adrress of this variable
    movs    r0, #1                        @ Write 1 to r0
    str     r0, [r2]                      @ Store 1 in variable atimer
    pop     {pc}                          @ Pop the link register from the stack to the program counter

install_gpio_isr:
    ldr     r2, =(PPB_BASE + M0PLUS_VTOR_OFFSET)       @ Adds the base address of the CPU in RP2040 to vector offset address to find out where vector table is
    ldr     r1, [r2]                                   @ Hexadecimal address of where the vector table(value stored in r2) should be loaded in r1. r1 now has actual vector table base address
    movs    r2, #GPIO_ISR_OFFSET                       @ Move offset address to r2
    add     r2, r1                                     @ Adds offset address to the base address, stores the whole value in r2. r2 now has the address to the service routine
    ldr     r0, =gpio_isr                              @ Address of the subroutine gpio_isr is stored in r0
    str     r0, [r2]                                   @ Installs the subroutine where we have the offset address of the table
    ldr     r0, =8192                                  @ Activate the 14th bit in the table
    ldr     r2, =(PPB_BASE + M0PLUS_NVIC_ICPR_OFFSET)  @ Adds the base address of the CPU in RP2040 to NVIC ICPR offset address
    str     r0, [r2]                                   @ Disable the GPIO IRQ by writing 8192 to the procured address
    ldr     r2, =(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET)  @ Adds the base address of the CPU in RP2040 to NVIC ISER offset address
    str     r0, [r2]                                   @ Disable the GPIO IRQ by writing 8192 to the procured address
    bx      lr                                         @ Branch and exchange with the last instruction in link register

.thumb_func                                                     @ Required for all interrupt service routines
gpio_isr:
    push    {lr}                                                @ Store the link register to the stack as we will call nested subroutines
    bl      watchdog_update                                     @ Prevent watchdog reset
    ldr     r2, =(IO_BANK0_BASE + IO_BANK0_PROC0_INTS2_OFFSET)  @ Get the address of the register which informs if an interupt happened
    ldr     r1, [r2]                                            @ Load the value from that register into r1
    ldr     r0, =GPIO_BTN_DN_MSK                                @ mask for falling edge
    cmp     r0, r1                                              @ Checks for falling edge in the registers
    beq     falling_edge                                        @ Go to the subroute if it is falling edge
    ldr     r0, =GPIO_BTN_UP_MSK                                @ Mask for rising edge
    cmp     r0, r1                                              @ Checks for rising edge the registers
    beq     rising_edge                                         @ Go to the subroute if it is rising edge
gpio_isr_done:
    bl      start_timer                                         @ Detect time to next interrupt
    pop     {pc}                                                @ Pop the link register from the stack to the program counter

falling_edge:                                               @ Subroutine to distinguish a dot or space when putton is pressed (quick release)
    ldr     r2, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET)    @ Get the address of the INTR Register 2
    ldr     r1, =GPIO_BTN_DN_MSK                            @ Get the mask for GP21 Falling Edge
    str     r1, [r2]                                        @ Reset the GPIO ISR for GP21 Falling Edge
    bl      end_timer                                       @ Calls on c funtion end timer to return time elapsed                              
    movs    r1, #1                                          @ Stores time elapsed from timer(1 - 1 second)
    cmp     r0, r1                                          @ Checks if one second has passed
    bgt     add_space                                       @ If time elasped is greater than go to add_space subroutine 
    b       add_dot                                         @ Else go to add_dot subroutine

rising_edge:                                                @ Subroutine to distinguish a dash when button is pressed (held)
    ldr     r2, =(IO_BANK0_BASE + IO_BANK0_INTR2_OFFSET)    @ Get the address of the INTR Register 2
    ldr     r1, =GPIO_BTN_UP_MSK                            @ Get the mask for GP21 Rising Edge
    str     r1, [r2]                                        @ Reset the GPIO ISR for GP21 Rising Edge
    bl      end_timer                                       @ Calls on c funtion end timer to return time elapsed
    movs    r1, #1                                          @ store 1 - 1 second
    cmp     r0, r1                                          @ Checks if one second has passed
    bgt     add_dash                                        @ If time elapsed is greater than go to add_dash subroutine
    b       gpio_isr_done                                   @ Else finish handling the interrupt

add_dot:                                                    @ Add dot subroutine
    movs    r0, #0                                          @ In add_input functoin in c, code for dot is 0
    movs    r1, #0                                          @ Sets the next character to be inputted at next index
    bl      add_input                                       @ Call the c function to add inputs
    b       gpio_isr_done                                   @ finish handlind the interrupt

add_space:                                                  @ Add space subroutine
    movs    r0, #2                                          @ In add_input function in c, code for space is 2
    movs    r1, #0                                          @ Sets the next character to be inputted at next index
    bl      add_input                                       @ Call the c function to add inputs
    b       add_dot                                         @ After a space is added always add a dot, which can be overwritten if next input is dash

add_dash:                                                   @ Add dash subroutine by replacing last dot
    movs    r0, #1                                          @ In add_input function in c, code for dash is 1
    movs    r1, #1                                          @ Sets the last character to be overwritten at previous index (i.e., the last dot)
    bl      add_input                                       @ Call the c function to add inputs
    b       gpio_isr_done                                   @ Finish handling the interrupt

.align 4
wlcm_msg:    .asciz "\n\t\tUse the GPIO 21 button to input your Morse Code.\n"      @ Message when GP22 is pressed
inpt_msg:    .asciz "\n\t\tYour Input: "
@ Set data alignment
.data
    .align 4
    ltimer:         .word   INPUT_END_TIME  @ Stores the current alarm time
    atimer:         .word   INITIAL_STATE   @ Initially alarm has not been run
