#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

static uint8_t HEIGHT = 9;
static uint8_t WIDTH = 9;

typedef struct field{
    uint8_t n_mines;        // num of mines around filed ( if 9 then is bomb )
    bool    is_selected;    // true if field has been selected     
    bool    is_flaged;      // ture if field has been flaged
} field;

typedef struct cursor{
    uint8_t x;
    uint8_t y;
} cursor;

//sets color acording to given field, cursor and field postion
//if not selected or hovered by cursor sets white bg and white fg
//if not selected but hovered by cursor set yellow bg and yellow fg
//if selected but not hoverd by cursor sets white bg and black fg
//if selected and hoverd by cursor sets yellow bg and black fg
void set_color(field f, cursor c, uint8_t x, uint8_t y){
    uint8_t fg = 255, bg = 255;

    if(f.is_selected || f.is_flaged){
        fg = 0;
    }

    if(c.x == x && c.y == y){
        bg = 3;
        if(fg == 255){
            fg = bg;
        }
    }

    printf("\033[38;5;%um\033[48;5;%um", fg, bg);
}

// counts the number of mines around gameboard[x][y]
uint8_t bombs_around_field(field** gameboard, int8_t x, int8_t y){
    uint8_t n = 0;

    int8_t i, j;

    for(i = x - 1; i <= x + 1; i++){

        if(i < 0 || i >= HEIGHT){
            continue;
        }

        for(j = y - 1; j <= y + 1; j++){

            if(j < 0 || j >= WIDTH || (j == y && i == x)){
                continue;
            }

            if(gameboard[i][j].n_mines == 9){
                n++;
            }

        }

    }

    return n;
}

field** init_fileds(uint8_t mines){

    field** gameboard = malloc(sizeof(field*) * HEIGHT);

    int8_t i, j;
    uint8_t x, y;

    for(i = 0; i < HEIGHT; i++){
        gameboard[i] = malloc(sizeof(field) * WIDTH);
    }

    srand(time(NULL));

    // set mines
    for(i = 0; i < mines; i++){
        do{
            x = rand() % HEIGHT;
            y = rand() % WIDTH;
        }while(gameboard[x][y].n_mines == 9);
        gameboard[x][y].n_mines = 9;
    }

    // calculate num of mines around each field
    for(i = 0; i < HEIGHT; i++){
        for(j = 0; j < WIDTH; j++){
            if(gameboard[i][j].n_mines == 9){
                continue;
            }
            gameboard[i][j].n_mines = bombs_around_field(gameboard, i, j);
        }
    }

    return gameboard;
}

void free_fields(field** gameboard){
    uint8_t i;

    for(i = 0; i < HEIGHT; i++){
        free(gameboard[i]);
    }

    free(gameboard);
}

void print_gameboard(field** gameboard, cursor c){
    uint8_t i, j;
    printf("\033[H"); // move cursor to 0, 0

    for(i = 0; i < HEIGHT; i++){
        for(j = 0; j < WIDTH; j++){
            set_color(gameboard[i][j], c, i, j);

            if(gameboard[i][j].is_flaged){
                printf("F");
                continue;
            }
            if(gameboard[i][j].n_mines == 9){
                printf("M");
                continue;
            }
            if(gameboard[i][j].n_mines == 0){
                printf(".");
                continue;
            }
            printf("%u", gameboard[i][j].n_mines);
        }
        printf("\n");
    }
}

// set buffered input
void set_buffered_input(bool enable){
    static bool enabled = true;
    static struct termios old;
    struct termios new;

    if(enable && !enabled){
        //restore old setting
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        enabled = true;
    }else if(!enable && enabled){
        // get the terminal settings for standard input
        tcgetattr(STDIN_FILENO, &new);
        old = new;

        // disable canonical mode (buffered i/o) and local echo
        new.c_lflag &= (~ICANON & ~ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &new);

        enabled = false;
    }
}

void signal_callback_handler(int signum){
    printf("\n");
    set_buffered_input(true);
    exit(signum);
}

int main(){

    printf("\033[2J"); // clear screen

    signal(SIGINT, signal_callback_handler);
    set_buffered_input(false);

    field** gamebaord = init_fileds(10);
    cursor game_cursor;
    game_cursor.x = 0;
    game_cursor.y = 0;

    print_gameboard(gamebaord, game_cursor);

    getchar();
    
    printf("\033[2;0;0m\n"); // reset color
    free_fields(gamebaord);
    set_buffered_input(true);

    return 0;
}
