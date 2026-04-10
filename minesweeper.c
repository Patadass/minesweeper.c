#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define GAMEOVER 1
#define WIN 2
#define OK 0

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

// recursively go through all fields that have 0 mines
// and are next to eachother
void select_field(field** gameboard, int8_t x, int8_t y){
    int8_t i, j;

    if(gameboard[x][y].n_mines != 0){
        return;
    }

    //these for loops are stupid
    for(i = x - 1; i <= x + 1; i++){
        if(i < 0 || i >= HEIGHT){
            continue;
        }
        for(j = y - 1; j <= y + 1; j++){
            if(j < 0 || j >= WIDTH || (j == y && i == x) || 
                    gameboard[i][j].is_selected){
                continue;
            }

            if((i == x - 1 && j == y - 1) || (i == x + 1 && j == y + 1) ||
                    (i == x + 1 && j == y - 1) || (i == x - 1 && j == y + 1)){
                if(gameboard[i][j].n_mines > 0){
                    gameboard[i][j].is_selected = true;
                }
            }else{
                gameboard[i][j].is_selected = true;
                select_field(gameboard, i, j);
            }

        }
    }

}


uint8_t run_game_logic(field** gameboard){
    uint8_t i, j;
    uint8_t code = OK;

    uint8_t to_be_selected = 0;
    uint8_t selected = 0;

    for(i = 0; i < HEIGHT; i++){
        for(j = 0; j < WIDTH; j++){
            if(gameboard[i][j].n_mines > 0 && gameboard[i][j].n_mines < 9){
                to_be_selected++;
            }
            if(!gameboard[i][j].is_selected){
                continue;
            }
            if(gameboard[i][j].n_mines == 9){
                return GAMEOVER;
            }
            if(gameboard[i][j].n_mines > 0){
                selected++;
            }
        }
    }

    if(selected >= to_be_selected){
        code = WIN;
    }

    return code;
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

    printf("\033[2;0;0m"); // reset color
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

    int8_t c = 0;
    bool leave = false;
    while(true){
        
        print_gameboard(gamebaord, game_cursor);

        printf("move:       wasd, hjkl or ← ↑ ↓ →\n");
        printf("flag:       f or space\n");
        printf("select:     enter\n");
        printf("quit:       q\n");

        c = getchar();

        switch(c){
            case 'd':
                 if(game_cursor.y + 1 < WIDTH){
                     game_cursor.y++;
                 }
            break;
            case 'a':
                if(game_cursor.y > 0){
                    game_cursor.y--;
                }
            break;
            case 'w':
                 if(game_cursor.x > 0){
                     game_cursor.x--;
                 }
            break;
            case 's':
                if(game_cursor.x + 1 < HEIGHT){
                    game_cursor.x++;
                }
            break;
            case 10: // return
                if(gamebaord[game_cursor.x][game_cursor.y].is_flaged){
                    break;
                }
                if(gamebaord[game_cursor.x][game_cursor.y].n_mines == 0 &&
                        !gamebaord[game_cursor.x][game_cursor.y].is_selected){
                    select_field(gamebaord, game_cursor.x, game_cursor.y);
                }
                gamebaord[game_cursor.x][game_cursor.y].is_selected = true;
            break;
            case 32:
            case 'f':
                if(gamebaord[game_cursor.x][game_cursor.y].is_flaged){
                    gamebaord[game_cursor.x][game_cursor.y].is_flaged = false;
                    break;
                }
                if(!gamebaord[game_cursor.x][game_cursor.y].is_selected){
                    gamebaord[game_cursor.x][game_cursor.y].is_flaged = true;
                }
            break;
            case 'q':
                printf("QUIT? [y,n]");
                c = getchar();
                if(c == 'y'){
                    leave = true;
                }else{
                    printf("\033[1E\033[1A%11s", " "); // write over QUIT? [y,n]
                }
            break;
            default:
            break;
        }

        if(leave){
            break;
        }

        uint8_t code = run_game_logic(gamebaord);
        if(code == GAMEOVER){
            print_gameboard(gamebaord, game_cursor);
            printf("GAME OVER\n");
            break;
        }
        if(code == WIN){
            print_gameboard(gamebaord, game_cursor);
            printf("YOU WIN\n");
            break;
        }
    }
    
    printf("\033[2;0;0m\n"); // reset color
    free_fields(gamebaord);
    set_buffered_input(true);

    return 0;
}
