#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "assign02.pio.h"
#include <time.h>

/**
 *  Will use RGBW format.
 */
#define IS_RGBW true        

/**
 *  There is 1 WS2812 device in the chain.
 */
#define NUM_PIXELS 1  

/**
 *  The GPIO pin that the WS2812 connected to.
 */
#define WS2812_PIN 28 

/**
 *  Maximum size for input string of morse name an instance of Morse struct.
 */
#define MAX_STRING_SIZE 20

/**
 *  Array size for hash table, prime number to avoid collisions.
 */
#define ARRAY_SIZE 173

/**
 *  Flag for setting game levels, if 1 display morse equivlents.
 */
int disp_morse = 1;

/**
 *  Flag for setting game levels, if 1 display letter equivlents.
 */
int letter = 1;

/**
 *  Corresponds to users lives, added or deducted based on correct or incorrect inputs respectively.
 */
int lives = 3;

/**
 *  store victory streak
 */
int victory_count=0; 

/**
 *  Corresponds to users lives, added or deducted based on correct or incorrect inputs respectively.
 */
int lives = 3;

/**
 *  Used for calculatesStats function, sums total attempts over correct attempts.
 */
int right_input;

/**
 *  Used for calculatesStats function, sums total attempts over correct attempts.
 */
int wrong_input;
 
/**
 *  Used for add input function, sets the max length on inputs.
 */
char current_input[200];

/**
 *  Used for add input function
 */
int current_index = -1;

/**
 *  For alarm timer
 */
absolute_time_t start_time;

/**
 *  Initialise struct and its sub classes.
 */
typedef struct Morse Morse;
struct Morse{
    char morse_name[MAX_STRING_SIZE];
    char letter;
};

/**
 * @brief Function called on to generate morse code for each word & 
 *        letter that the user will input, by allocating space of size 
 *        Morse struct for the new morse deatils specified.
 * 
 * @param  name       Will set the morse_name in Morse struct to the  
 *                    name specified when the funciton was called.
 * @param  letter     Will set the letter in Morse struct to the  
 *                    letter specified when the funciton was called.
 *
 * @return new_morse  Returns the new morse struct created.
 */
Morse* createMorse(char* name, char letter){
    Morse* new_morse = (Morse*) malloc(sizeof(Morse));
    strcpy(new_morse->morse_name, name);
    new_morse->letter = letter;
    return new_morse;
}

/**
 *  @ stores the random letter generated from the init random 
 *    morse function.
 */
Morse* rand_array[36];

char rand_letters[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

/**
 *  @ passing the aphanumeric values to help in the generation of 
 *    random values for the morse pattern.
 */
char letters[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

/**
 *  @ stores the corresponding morse values for the aplahbets from A 
 *    to Z and numbers from 0 to 9.
 */
char morse_data[36][6] = {
    {".-"},
    {"-..."},
    {"-.-."},
    {"-.."},
    {"."},
    {"..-."},
    {"--."},
    {"...."},
    {".."},
    {".---"},
    {"-.-"},
    {".-.."},
    {"--"},
    {"-."},
    {"---"},
    {".--."},
    {"--.-"},
    {".-."},
    {"..."},
    {"-"},
    {"..-"},
    {"...-"},
    {".--"},
    {"-..-"},
    {"-.--"},
    {"--.."},
    {".----"},
    {"..---"},
    {"...--"},
    {"....-"},
    {"....."},
    {"-...."},
    {"--..."},
    {"---.."},
    {"----."},
    {"-----"},
};

/**
 * @brief   function to generate random letters/words and it's morse 
 *          equivalent for the user to input by passing parameters to 
 *          createMorse function.
 */
void init_random_morse(){
    for(int i = 0; i < 36; i ++){
        rand_array[i] = createMorse(morse_data[i], letters[i]);
    }
}

/**
 * @brief   function called on in the fisher yates algorithm to help
 *          generate the random permutation.
 */
void swap_array(int x, int y){ 
    Morse* temp = rand_array[x];
    rand_array[x] = rand_array[y];
    rand_array[y] = temp;
}

/**
 * @brief   function to generate the random permutation for the finite 
 *          sequence provided to it.
 */
void fisher_yates_36(){
    init_random_morse();
    srand(time(NULL));
    absolute_time_t time = get_absolute_time();
    int randgen = to_ms_since_boot(time);
    srand(randgen);
    for(int i = 0; i < 36; i++){
        int x = rand() % 36;
        swap_array(i, x);
    }
}

/**
 *  @ Storing the corresponding morse for each alphabet and for each number  
 *    in the hash table and this hash table is also used by the user to 
 *    compare the morse value inputes by the user.
 */
Morse* hashTable[ARRAY_SIZE];

/**
 * @brief   function takes string s as a parameter and generates a hash
 *          value out of given ascii string, used to assign letters and
 *          numbers index's for the hashtable.
 *
 * @param   Some letter or number taken as input string.
 *
 * @return  Returns the hash (index number) for the given input.
 */
int hash_func(char* s){
    int hash = 0;
    while(*s){
        hash = 31 * (hash + *s) % ARRAY_SIZE;
        s++;
    }
    return hash;
}

/**
 * @brief   function used to search the hash table by calling hashing
 *          function to retrive a letter or numbers respective index.
 *
 * @param   Some letter or number taken as input string
 *
 * @return  returns an index number for the given input
 */
int intsearch (char* name){
    int hash_num = hash_func(name);
    int start = hash_num;
    while(hashTable[hash_num] != NULL){
        if(strcmp(hashTable[hash_num]->morse_name, name)==0){
            return hash_num;
        }
    hash_num = hash_num + 1;
        if(hash_num == ARRAY_SIZE){
            hash_num = 0;
        }
        if(hash_num == start){
            return -1;
        }
    }
    return hash_num;
}

/**
 * @brief   Function initialises the hash table by inserting the 
 *          alphanumeric values into the hash table till the key 
 *          values of hash table gets exhausted (will never happen).
 */
void insert(){
    for(int i = 0; i < 36; i++){
        int hash_num = intsearch(morse_data[i]);
        if(hash_num == -1){
            printf("Warning! Hash table is full upon loading!");
        }
        else{
            Morse* new_morse = createMorse(morse_data[i], letters[i]);
            hashTable[hash_num] = new_morse;
        }
    }
}

/**
 * @brief This function starts the timer when the alarm interupt is
 *        called.
 */
void start_timer(){
    start_time = get_absolute_time();
}

/**
 * @brief This function returns an int which represents the time
 *        elapsed of the alarm interupt when the button is pressed. 
 */
int end_timer(){
    int end = (int) absolute_time_diff_us(start_time, get_absolute_time());
    return end/100000;
}

/**
 * @brief The Add input function is called upon in various nested 
 *        subroutines in the ARM code to add the interpreted input 
 *        as a dot, dash, space, or null charcater. It will also 
 *        increment an index counter each time the function is 
 *        called to keep track of inputted letters/words.
 *
 * @param sample: sets the input as either a dot (0), dash (1), 
 *        space (2), or end of string input (3).
 * @param correct_index: sets the input added as new index (0)
 *        or as the last index (1) in the case of adding a dash,
 *        this is done to overwrite the dot that is the default.      
 */
void add_input(int sample, int correct_index){
    current_index = current_index - correct_index;
    if (current_index < 200){
        if (sample == 0){
            current_input[current_index] = '.';
        }
        else if (sample == 1){
            current_input[current_index] = '-';
        }
        else if (sample == 2){
            current_input[current_index] = ' ';
        }
        else if(sample == 3){
            current_input[current_index] = '\0';
        }
    }
    current_index++;
    if (correct_index){
        printf("\b%c",current_input[current_index-1]);
    }
    else printf("%c",current_input[current_index-1]);

}

/** 
 * @brief Must declare the main assembly entry point before use.
 */
void main_asm();

/**
 * @brief Initialise a GPIO pin – see SDK for detail on gpio_init()
 */
void asm_gpio_init(uint pin) {
    gpio_init(pin);
}

/**
 * @brief Set direction of a GPIO pin – see SDK for detail on gpio_set_dir()
 */
void asm_gpio_set_dir(uint pin, bool out) {
    gpio_set_dir(pin, out);
}

/**
 * @brief Get the value of a GPIO pin – see SDK for detail on gpio_get()
 */
bool asm_gpio_get(uint pin) {
    return gpio_get(pin);
}

/**
 * @brief Set the value of a GPIO pin – see SDK for detail on gpio_put() 
 */
void asm_gpio_put(uint pin, bool value) {
    gpio_put(pin, value);
}

/**
 * @brief Set the falling and rising edge Interrupts for the pin specified
 * 
 * @param pin Pin Number to set the interrupt on
 */
void asm_gpio_set_irq(uint pin){
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}

/**
 * @brief Wrapper function used to call the underlying PIO
 *        function that pushes the 32-bit RGB colour value
 *        out to the LED serially using the PIO0 block. The
 *        function does not return until all of the data has
 *        been written out.
 * 
 * @param pixel_grb The 32-bit colour value generated by urgb_u32()
 */
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

/**
 * @brief Function to generate an unsigned 32-bit composit GRB
 *        value by combining the individual 8-bit paramaters for
 *        red, green and blue together in the right order.
 * 
 * @param r     The 8-bit intensity value for the red component
 * @param g     The 8-bit intensity value for the green component
 * @param b     The 8-bit intensity value for the blue component
 * @return uint32_t Returns the resulting composit 32-bit RGB value
 */
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return  ((uint32_t) (r) << 8)  | ((uint32_t) (g) << 16) | (uint32_t) (b);
}

/**
 * @brief function sets LED off. 
 */
void set_color_led_off(){
    put_pixel(urgb_u32(0x00, 0x00, 0x00));
}

/**
 * @brief function sets LED red. 
 */
void set_red_on(){
    put_pixel(urgb_u32(0x7F, 0x00, 0x00));
}

/**
 * @brief function sets LED green. 
 */
void set_green_on(){
    put_pixel(urgb_u32(0x00, 0x7F, 0x00));
}

/**
 * @brief function sets LED yellow. 
 */
void set_yellow_on(){
    put_pixel(urgb_u32(0x0F,0x0F,0x0F));
}

/**
 * @brief function sets LED blue. 
 */
void set_orange_on(){
    put_pixel(urgb_u32(0xFF, 0xA5, 0x00));
}

/**
 * @brief function sets LED blue. 
 */
void set_blue_on(){
    put_pixel(urgb_u32(0x00, 0x00, 0x7F));
}

/**
 * @brief A function called upon in start the game function that 
 *        displays a rainbow coloured led sequnce to highlight the
 *        game is complete. 
 */
void winning_sequence(){
    while(true){
        set_blue_on();
        sleep_ms(500);
        set_green_on();
        sleep_ms(500);
        set_yellow_on();
        sleep_ms(500);
        set_orange_on();
        sleep_ms(500);
        set_red_on();
    }
}

/**
 * @brief A function which is called upon anytime an input is
 *        is verified as correct or incorrect to notfiy the user 
 *        of their lives.
 */
void set_correct_led(){
    if (lives == 1)set_orange_on();
    else if (lives == 2)set_yellow_on();
    else set_green_on();
    return ;
}

/**
 * @brief A function called upon in start the game function that 
 *        calculates your overall accuracy throughout the game by 
 *        summing your total attempts over the total correct attempts. 
 */
void calculateStats(int reset){
    printf("\n\n\t\t****************Scoreboard****************");
    printf("\n\t\t*Attemps: \t\t\t\t%d*", right_input+wrong_input);
    printf("\n\t\t*Correct: \t\t\t\t%d*", right_input);
    printf("\n\t\t*Incorrect: \t\t\t\t%d*", wrong_input);
    printf("\n\t\t*Win Streak: \t\t\t\t%d*", victory_count);
    printf("\n\t\t*Lives Left: \t\t\t\t%d*\n", lives);
    if (right_input!=0 || wrong_input!=0){
        float stat = right_input * 100 / (right_input + wrong_input);
        if (reset){
            right_input = 0;
            wrong_input = 0;
            printf("\t\t*Correct %% for this level: \t%.2f%%*\n",stat); 
        }
        else {
            printf("\t\t*Correct Percent :\t\t\t%.2f%%*\n",stat);
        }
    }
    printf("\t\t****************Scoreboard****************\n\n");
}

/**
 * @brief A function called upon in start the game function that 
 *        sets the difficulty (level) of the game based on an integer  
 *        deciding which features of the game to activate. If 
 *        disp_morse is 1: the corresponding letter should have morse 
 *        input on display. If 0 it should not. Same goes for letter.
 */
int level(int l){
    if (l == 1){
        disp_morse = 1;
        letter = 1;
    }
    else if (l == 2){
        disp_morse = 0;
        letter = 1;
    }
    else if (l == 3){
        disp_morse = 1;
        letter = 0;
    }
    else if (l == 4){
        disp_morse = 0;
        letter = 0;
    }
    return l;
}

/**
 * @brief This function is called at the beginning of the game and
 *        prompts the user with which level they would like to attempt,
 *        sets some variables needed, and then prompt the user with a 
 *        random letter/number or morse symbol depending on spcified level. It
 *        will also track an index of correct or incorrect inputs adding
 *        or deducting lives respectively. Calculate statistics based on 
 *        inputs.
 */
void start_the_game(){
    set_blue_on();
    printf("\n\n\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("| #               # # # # # # #          #          # # # # # #    #           # |\n");
    printf("| #               #                    #   #        #           #  # #         # |\n");
    printf("| #               #                  #       #      #           #  #   #       # |\n");
    printf("| #               # # # # #        #           #    # # # # # #    #     #     # |\n");
    printf("| #               #               # # # # # # # #   #      #       #       #   # |\n");
    printf("| #               #               #             #   #        #     #         # # |\n");
    printf("| # # # # # # #   # # # # # # #   #             #   #          #   #           # |\n");
    printf("|                                                                                |\n");
    printf("|                       --. .-. --- ..- .--. / ----- -....                       |\n");
    printf("|                                                                                |\n");
    printf("| #           #   ");    printf("# # # # # # #  ");     printf("  # # # # # #  ");   printf("     # # # # #     ");       printf("# # # # # # # | \n"); 
    printf("| # #       # #   ");    printf("#           #  ");     printf("  #          # ");   printf("    #          #   ");       printf("#             |\n");
    printf("| #   #   #   #   ");    printf("#           #  ");     printf("  #          # ");   printf("    #              ");       printf("#             | \n");
    printf("| #     #     #   ");    printf("#           #  ");     printf("  # # # # # #  ");   printf("     # # # # #     ");       printf("# # # # #     | \n");
    printf("| #           #   ");    printf("#           #  ");     printf("  #       #    ");   printf("               #   ");       printf("#             | \n");
    printf("| #           #   ");    printf("#           #  ");     printf("  #        #   ");   printf("    #          #   ");       printf("#             |  \n");
    printf("| #           #   ");    printf("# # # # # # #  ");     printf("  #         #  ");   printf("     # # # # #     ");       printf("# # # # # # # | \n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("\n\n\n\n\n\n\t\t***********************************************\n");
    printf("\t\t*                                             *\n");
    printf("\t\t* Enter .---- for level 1 (Characters - Easy) *\n");
    printf("\t\t* Enter ..--- for level 2 (Characters - Hard) *\n");
    printf("\t\t* Enter ...-- for level 3 (Words - Easy)      *\n");
    printf("\t\t* Enter ....- for level 4 (Words - Hard)      *\n");
    printf("\t\t*                                             *\n");
    printf("\t\t***********************************************\n");
    current_index = -1;
    main_asm();
    printf("\n\n\n\n\n\n");
    int current_level;
    if (strcmp(current_input, ".----") == 0){
        current_level = level(1);
    }
    else if (strcmp(current_input, "..---") == 0){
        current_level = level(2);
    }
    else if (strcmp(current_input, "...--") == 0){
        current_level = level(3);
    }
    else if (strcmp(current_input, "....-") == 0){
        current_level = level(4);
    }
    else {
        printf("\t\tWrong Input\n\t\tProgram will now exit\n");
        return;
    }
    set_correct_led();
    int num_count = rand() % 36;
    char current_letter;
    char* current_morse;
    char current_word[4];
    char current_morse2[50] = "";
    char space[2] = " \0";

    while(lives > 0){
        printf("\n\n\n\n\n\n\n\n\n\n\n");
        if(letter == 1){
            current_letter = rand_array[num_count]->letter;
            current_morse = rand_array[num_count]->morse_name;
            if(disp_morse == 1){ 
                printf("\n\t\tYour Challenge is: %c \n\t\tand %c in Morse is %s:\n", current_letter, current_letter,current_morse);
            }
            else
                printf("\n\t\tYour Challenge is: %c\n", current_letter);
        }
        else {
            for (int i = 0; i < 3; i++){
                current_word[i] = rand_array[num_count]->letter;
                strcat(current_morse2, rand_array[num_count]->morse_name);
                if(i != 2){
                    strcat(current_morse2, space);
                }
                num_count = rand() % 36;
            }
            current_word[3] = '\0';
            current_morse = current_morse2;
            if(disp_morse == 1){
                printf("\n\t\tYour Challenge is: %s \n\t\tand %s in Morse, it's %s:\n", current_word, current_word, current_morse);
            }
            else
                printf("\n\t\tYour Challenge is: %s\n", current_word);
        }
        printf("\t\tEnter the Morse Code after the prompt\n");
        current_index = -1;
        main_asm();
        if (current_index == 0){
            printf("\t\tNo update detected\nGenerating New Level\n");
            continue;
        }
        int pos = intsearch(current_input);
        if(strcmp(current_morse,current_input)==0){
            right_input++;
            victory_count++;
            printf("\t\tCorrect!\n");
            if (lives < 3)lives = lives + 1;
            if (victory_count == 5){
                if (current_level == 4){
                    printf("\t\tCongrats, you have completed the Game!\n");
                    winning_sequence();
                    break;
                }
                current_level = level(current_level+1);
                printf("\n\n\t\t################################################################\n\n");
                printf("\t\t\t\tAdvancing to Level %d\n\n",current_level);
                printf("\t\t################################################################\n\n");
                calculateStats(1);
                victory_count = 0;
            }
        }
        else{
            wrong_input++;
            printf("Incorrect :(\n");
            if (strcmp(hashTable[pos]->morse_name, "")==0){
                printf("\t\tMorse Code does not exist\n");
            }
            else if (letter){
                printf("\t\tMorse Code you entered is for %c\n",hashTable[pos]->letter);
            }
            lives = lives - 1;
            victory_count = 0;
        }
        num_count = rand() % 36;
        set_correct_led();
        current_input[0] = '\0';
        current_morse2[0] = '\0';
        if (right_input+wrong_input)calculateStats(0);
    }
    calculateStats(1);
    if(lives == 0)set_red_on();
    printf("\n\n\n\n\n\n\t\t*****************************\n");
    printf("\t\t*                           *\n");
    printf("\t\t* Enter .---- to play again *\n");
    printf("\t\t* Enter ..--- to exit       *\n");
    printf("\t\t*                           *\n");
    printf("\t\t*****************************\n");
    current_index = -1;
    main_asm();
    if (current_index == 0){
        printf("\t\tNo update detected\n\t\tProgram Will now Exit.");
    }
    if (strcmp(current_input, ".----") == 0){
        start_the_game();
    }
}

/**
 * @brief function to free memory allocated with regards to the hashtable.
 */
void dealloc(){
    for(int i = 0; i < ARRAY_SIZE; i++){
        free(hashTable[i]);
    }
    for(int i = 0; i < 36; i++){
        free(rand_array[i]);
    }
}

/**
 * @brief initialises the application
 * 
 * @return int return 0 to indicate everything went okay
 */
int main() {

    stdio_init_all();
    
    // Initialise the PIO interface with the WS2812 code
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);
    fisher_yates_36(); 
    insert();
    watchdog_enable(0x7fffff, 1);
    start_the_game();
    dealloc();
    printf("\t\tGame has ended\n");
    return(0);
}
