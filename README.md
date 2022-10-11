## Abstract:

This assignment focuses on a mixture of C code and ARM assembly. I was tasked with implementing a program on the MAKER-PI-PICO board that could teach a user morse code using a series of buttons on the board as input signals processed by the code. I managed to implement all features required in the project brief including some bonus levels 3 and 4, the use of Git was also a necessary feature to complete the project.

## Introduction: 

This section of the report breaks down the project tasks and how the application will work at a high level.
1. Firstly a main function will be required which will display a welcome message and set the LED blue, along with prompting the user to choose a level (minimum requirement is 2 levels). 
2. Once a user has specified a level via some input, the program should set the LED green and then prompt the user with a series of characters to be typed inputted as morse (with/without their morse code equivalent depending on the level). 
3. These inputs should then be verified and for each incorrect input a life should be deducted and the LED set orange or red for 2 or 1 lives respectively. 
4. Once the lives reach 0 the LED should be set red (indicate that the game is over). 
5. For correct inputs these should be counted and once the system detects 5 correct inputs, the program should progress to the next level. A life is added for each correct input too (max 3 lives). 
6. While the program is running in any given state a timer should be implemented that if after 9 seconds nothing is inputted it resets.

**Description of the application at a high level:**

Here the project design is visualised through various C functions.

Initially main runs, setting up a hash data structure to store retrieve various morse codes and their equivalents from a predefined array. Once set up, main calls on a function to start the game. This function first sets the LED blue, prompts the user with level choice, and calls on the main ASM code, which will display a welcome message and take the following input that corresponds to the specified level.
The level function will take this integer and set the corresponding difficulties accordingly. If an invalid input is detected the will application exit.

Once a level is set, the start game function then sets the LED green and initialises the variables needed for the calculateStats function and input comparisons. The game will then work back and forth with the main ASM function to interpret the appropriate inputs and progress through the 4 levels, and use the set_correct_led, winning_sequence to determine the correct colour configurations with respect to the set level. The calculateStats function should keep count of correct and total inputs to display the user’s accuracy. Once either the user runs out of lives setting the LED is set red, or the user completes the game setting the winning sequence, and an appropriate exit message displaying along with deallocating any memory used.


## Code:

This section is broken into two parts. The first part will cover the ARM related coding features implemented, any issues, and tasks along the way, while the second part will cover the C related aspects of the code and also how they relate back to the ARM code.

**The ARM code:**
**main_asm**
~~~
main_asm:                          	        @ Entry point to the ASM portion of the program
    push     {lr}                       	@ Stores the last state from c code so it knows where to return
    bl       init_pin                   	@ Intialises pins (from previous labs)
    bl       install_alrm_isr           	@ Installs alarm (from previous labs)
    bl       install_gpio_isr           	@ Installs gpio (from previous labs)
    ldr      r0, =wlcm_msg              	@ Stores message in r0
    bl       printf                     	@ And print using c function
~~~ 
This main sets up the ARM part of the application by first initialising all pins by branching to the init_pin subroutine, it then initialises the alarm interrupt and button via install_alrm_isr and install_gpio_isr respectively before displaying a welcome message. For this sub routine the following changes were made:

* At some point in the project once the C code executed the main_asm entry point for ARM code it would not return to C code. This was issue was resolved by adding ‘push {lr}’ to the beginning of subroutine.
* ...etc


**loop**
~~~ 
loop:
    bl      set_alarm                   	@ Loop to reset the alarm for 2 seconds after each input
    wfi			@ Wait for interrupt
    ldr     r2, =atimer                 	@ Variable will check if alarm interrupt has run or not
    ldr     r1, [r2]                    	@ Loading the variable value into r1
    movs    r0, #1                      	@ Put constant 1 in r0
    cmp     r0, r1                      	@ Comparison of variable atimer
    bne     loop                       	    @ Loop if not same (wait until equal)
    movs    r0, #0                      	@ Store 0 to reset ALARM Interrupt state for next input
    str     r0, [r2]                    	@ Update the value by storing it back in atimer variable
    movs    r0, #3                      	@ In add_input function in c, code for end of string is 3
    movs    r1, #0                      	@ Sets the next character in this case '\0' to be inputted at next index
    bl      add_input                   	@ Call the c function to add the input
    pop     {pc}                        	@ Return back to the next line of code in c
~~~
The majority of the ASM functionality was implemented here in the loop through various subroutines, and interrupts by comparing register values. This subroutine functions by essentially waiting for an interrupt triggered by button, which resets the alarm by checking if the alarm has run, if it has it exits.  . For thus subroutine the following changes were made: 

* some change if any...


**init_pin**
~~~
init_pin:
    push    {lr}                    @ Store the link register to the stack as we will call nested subroutines
    movs    r0, #GPIO_BTN_INPT      @ This value is the GPIO 20 pin on the PI PICO board
    bl      asm_gpio_init           @ Call the subroutine to initialise the GPIO pin specified by r0
    movs    r0, #GPIO_BTN_INPT      @ This value is the GPIO 20 pin on the PI PICO board
    bl      asm_gpio_set_irq        @ Call the subroutine to set up falling edge and rising edge on GPIO pin r0
    pop     {pc}                       	@ Pop the link register from the stack to the program counter
~~~
Directly from assignment 1, initialises button pin and sets it up to detect falling and rising edge of button. No issues here.


