#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include "gamelib.h"
#include "tankyRed.c"
#include "tankyBlue.c"
#include "text.h"

static const char *device = "/dev/fb1";

// Initializes all the pixels of a frame to the same color
void initFrame(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], uint16_t color)
{
	int x;
	int y;
	for(x=0;x<GRAPHICS_WIDTH;x++){
		for(y=0;y<GRAPHICS_HEIGHT;y++){
			frame[y][x] = color;
		}
	}
}

struct Rectangle{
        int x;
        int y;
        int width;
        int height;
        int speedX;
        int speedY;
        int color;
        int fired;
        int type;
};

void collideWall(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], struct Rectangle* rect){
	if((rect -> x + rect -> width) >= GRAPHICS_WIDTH || rect -> x <= 0){
		rect -> speedX *= -1;
	}
	if((rect -> y + rect -> height) >= GRAPHICS_HEIGHT || rect -> y <= 0){
		rect -> speedY *= -1;
	}
}

int collideRect(struct Rectangle* rect1, struct Rectangle* rect2){
	int coll = 0;
	if((rect1 -> x <= rect2 -> x + rect2 -> width) && (rect1 -> x >= rect2 -> x) && (rect1 -> y <= rect2 -> y + rect2 -> height) && (rect1 -> y >= rect2 -> y) ){
		coll = 1;
	}
    if((rect1->x <=rect2->x+rect2->width)&&(rect1->x>=rect2->x)&&(rect1->y+ rect1 -> height <= rect2 -> y + rect2 -> height) && (rect1 -> y  + rect1 -> height >= rect2 -> y) ){
		coll = 1;
	}
	if((rect1->x+rect1->width<=rect2->x+rect2->width)&&(rect1->x+rect1->width>=rect2->x)&&(rect1->y+rect1->height<=rect2->y+rect2 -> height) && (rect1 -> y  + rect1 -> height >= rect2 -> y)){ 
            coll = 1;
    }
	if((rect1 -> x + rect1 -> width <= rect2 -> x + rect2 -> width) && (rect1 -> x + rect1 -> width >= rect2 -> x) && (rect1 -> y <= rect2 -> y + rect2 -> height) && (rect1 -> y >= rect2 -> y)){
            coll = 1;
    }
	return coll;
}

// Renders a colored rectangle to the frame
void drawRect(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], struct Rectangle* rect)
{
	if(rect -> x < 0){
		rect-> x = 0;
	}
	if(rect -> y < 0){
		rect -> y = 0;
	}
	if(rect -> y + rect -> height > GRAPHICS_HEIGHT){
		rect -> y = GRAPHICS_HEIGHT - rect -> height;
	}
	if(rect -> x + rect -> width > GRAPHICS_WIDTH){
		rect -> x = GRAPHICS_WIDTH;
	}
	int y1 = rect -> y;
	int x1 = rect -> x;
	for(;x1 < rect -> x + rect -> width;x1++){
		for(y1 = rect -> y; y1 < rect -> y + rect -> height;y1++){
			frame[y1][x1] = rect -> color;
		}
	}
}

void drawImage(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], struct Rectangle* rect, const uint8_t *pixels){
    uint16_t *p = (uint16_t*) pixels;
	int y1 = rect -> y;
	int x1 = rect -> x;
	for(;x1 < rect -> x + rect -> width;x1++){
		for(y1 = rect -> y; y1 < rect -> y + rect -> height;y1++){
			frame[y1][x1] = p[(x1 - rect -> x) + ((rect -> width) * (y1 - rect -> y))];
		}
	}
}

void drawGround(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int ground[320]){
    int i = 0;
    int j = ground[i];
    for(;i<320;i++){
        frame[ground[i]][i] = 0x0F00;
        for(j=ground[i]; j<GRAPHICS_HEIGHT;j++){
            frame[j][i] = 0x0F00;
        }
    }
}

void drawHealth(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x, int y, int health){
    int i = 0,j = 0;
    char letter = '0' + health;
    for(;i<7; i++){
        for(j = 0;j<13; j++){
            if(charHasPixelSet(letter, j, i)){
                frame[y + j][x + i] = 0xFFFF;
            }
        }
    }
}

void drawString(uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH], int x, int y, const char s[], int color){
    int i = 0,j = 0, k = 0;
    while(s[k] != 0){
        for(i = 0;i<7; i++){
            for(j = 0;j<13; j++){
                if(charHasPixelSet(s[k], j, i)){
                    frame[y + j][x + i] = color;
                }
            }
        }
        x+=8;
        k++;
    }
}

int main(int argc, char **argv)
{
    int fd = open(device, O_RDWR);
    int colors = 0x0000;
    int state = '0';
    uint16_t frame[GRAPHICS_HEIGHT][GRAPHICS_WIDTH];
    enableInput();
    initTimer(20.0);
    initTimer(20.0);
    int speedX = 15;
    int speedY = -10;
    int player2Health = 5;
    int player1Health = 5;
    int player1Turn = 1;
    int hole;
    const char player1TurnString[14] = "Red turn";
    const char player2TurnString[14] = "Blue turn";
    const char player1WinsString[14] = "Red wins";
    const char player2WinsString[14] = "Blue wins";
    const char press12Start[17] = "Press 1 to start";
    const char instructions1[15] = "Red hate blue!";
    const char instructions2[15] = "Blue hate red!";
    const char instructions3[29] = "Use tank, kill what you hate";
    const char instructions4[24] = "Up and down aim bullets";
    const char instructions5[27] = "Numbers give other bullets";
    
  
    int ground[320];
    int i = 0;
    for (;i<320;i++){
        ground[i] = 200;
        if(i > 64 && i <= 128){
            ground[i] = 175;
        } else if(i > 128 && i <= 192){
            ground[i] = 150;
        } else if(i > 192 && i <= 256){
            ground[i] = 175;
        }
    }
    struct Rectangle* player1 = malloc(sizeof(struct Rectangle));
    player1 -> x = 10;
    player1 -> width = 30;
    player1 -> height = 30;
    player1 -> y = ground[player1 -> x] - player1 -> height;
    player1 -> speedX = 1;
    player1 -> speedY = 0;
    player1 -> color = 0xF000;
    player1 -> fired = 0;
    player1 -> type = 0;
    
    struct Rectangle* aim = malloc(sizeof(struct Rectangle));
    aim -> x = player1 -> x + speedX * 6;
    aim -> y = player1 -> y + speedY * 3;
    aim -> width = 10;
    aim -> height = 10;
    aim -> speedX = 0;
    aim -> speedY = 0;
    aim -> color = 0xF0F0;
    aim -> fired = 0;
    aim -> type = 0;
    
    struct Rectangle* player2 = malloc(sizeof(struct Rectangle));
    player2 -> x = 320 - 30 - 10;
    player2 -> width = 30;
    player2 -> height = 30;
    player2 -> y = ground[player2 -> x] - player2 -> height;
    player2 -> speedX = -1;
    player2 -> speedY = 0;
    player2 -> color = 0x00F0;
    player2 -> fired = 0;
    player2 -> type = 0;
    
    struct Rectangle* bullet = malloc(sizeof(struct Rectangle));
    bullet -> x = player1 -> x + 10;
    bullet -> y = player1 -> y + 10;
    bullet -> width = 10;
    bullet -> height = 10;
    bullet -> speedX = 0;
    bullet -> speedY = 0;
    bullet -> color = 0xF000;
    bullet -> fired = 0;
    bullet -> type = 1;
    
    struct Rectangle* explosion = malloc(sizeof(struct Rectangle));
    explosion -> x = 0;
    explosion -> y = 0;
    explosion -> width = 30;
    explosion -> height = 30;
    explosion -> speedX = 0;
    explosion -> speedY = 0;
    explosion -> color = 0xF0F0;
    explosion -> fired = 0;
    explosion -> type = 0;
    
    while(1){
        if(state != '3' && state != '1'){
            initFrame(frame, colors);
            drawString(frame, 50, 20, instructions1, 0xF000);
            drawString(frame, 50, 30, instructions2, 0x00F0);
            drawString(frame, 50, 40, instructions3, 0xF0F0);
            drawString(frame, 50, 50, instructions4, 0xF0F0);
            drawString(frame, 50, 60, instructions5, 0xF0F0);
            drawString(frame, 50, 70, press12Start, 0xFF00);
            state = getKeyPress();
            if(state == '3') {
                state = '0';
            }
            outputFrame(fd, frame);
        } else if(state == '3') {
            if(player1Health <= 0){
                drawString(frame, 100, 10, player2WinsString, 0x00F0);
            } else if(player2Health <= 0){
                drawString(frame, 100, 10, player1WinsString, 0xF000);
            } else {
                state = '1';
            }
            state = getKeyPress();
            if(state != '0' && state != '1'){
                state = '3';
            }
            outputFrame(fd, frame);
        } else if(state == '1'){
            if(player1Health <= 0 || player2Health <= 0){
                player1Health = 5;
                player2Health = 5;
            }
            int pressed = getKeyPress();
            if(pressed == 32){
                if(!bullet -> fired){
                    if(player1Turn){
                        bullet -> speedX = speedX;
                    } else {
                        bullet -> speedX = -speedX;
                    }
                    bullet -> speedY = speedY;
                    bullet -> fired = 1;
                }
            }
            else if(pressed == KEY_UP){
                speedY -= 1;
                speedX -= 1;
            }
            else if(pressed == KEY_DOWN){
                speedY += 1;
                speedX += 1;
            }
            else if(pressed == KEY_RIGHT){
                if(player1Turn){
                    player1 -> x += player1 -> speedX;
                    player1 -> y = ground[player1 -> x + player1 -> width]  - player1 -> height;
                    if (!bullet -> fired){
                        bullet -> x += player1-> speedX;
                        bullet -> y = player1 -> y;
                    }
                } else {
                    player2 -> x += player2 -> speedX;
                    player2 -> y = ground[player2 -> x]  - player2 -> height;
                    if (!bullet -> fired){
                        bullet -> x += player2-> speedX;
                        bullet -> y = player2 -> y;
                    }
                }  
            }
            else if(pressed == KEY_LEFT){
                if(player1Turn){
                    player1 -> x -= player1 -> speedX;
                    if (!bullet -> fired){
                        bullet -> x -= player1-> speedX;
                    }
                } else {
                    player2 -> x -= player2 -> speedX;
                    if (!bullet -> fired){
                        bullet -> x -= player2-> speedX;
                    }
                }
            }
        if(player1Turn){
            if(collideRect(bullet, player2)){
                player2Health -= 1;
                bullet -> speedX = 0;
                bullet -> speedY = 0;
                bullet -> x = player2 -> x + 10;
                bullet -> y = player2 -> y + 10;
                bullet -> fired = 0;
                speedX = 15;
                speedY = -10;
                player1Turn = 0;
            } else {
                bullet -> x -= bullet -> speedX / 2;
                if(collideRect(bullet, player2)){
                    player2Health -= 1;
                    bullet -> speedX = 0;
                    bullet -> speedY = 0;
                    bullet -> x = player2 -> x + 10;
                    bullet -> y = player2 -> y + 10;
                    bullet -> fired = 0;
                    speedX = 15;
                    speedY = -10;
                    player1Turn = 0;
                }
                bullet -> x += bullet -> speedX / 2;
            }
        } else {
            if(collideRect(bullet, player1)){
                player1Health -= 1;
                bullet -> speedX = 0;
                bullet -> speedY = 0;
                bullet -> x = player1 -> x + 10;
                bullet -> y = player1 -> y + 10;
                bullet -> fired = 0;
                speedX = 15;
                speedY = -10;
                player1Turn = 1;
            } else {
                bullet -> x += bullet -> speedX / 2;
                if(collideRect(bullet, player1)){
                    player2Health -= 1;
                    bullet -> speedX = 0;
                    bullet -> speedY = 0;
                    bullet -> x = player2 -> x + 10;
                    bullet -> y = player2 -> y + 10;
                    bullet -> fired = 0;
                    speedX = 15;
                    speedY = -10;
                    player1Turn = 0;
                }
                bullet -> x -= bullet -> speedX / 2;
            }
        }
        if(bullet -> y + bullet -> height > ground[bullet -> x] || bullet -> x >= 300 || bullet -> x <= 0){
            bullet -> speedX = 0;
            bullet -> speedY = 0;
            if(bullet -> y + bullet -> height > ground[bullet -> x]){
                explosion -> x = bullet -> x;
                explosion -> y = bullet -> y;
                explosion -> type = 1;
                for(hole = bullet -> x - 5;hole < bullet -> width + bullet -> x + 5; hole++){
                    if (explosion -> y + explosion -> height > ground[hole]){
                        ground[hole] = explosion -> y + explosion -> height;
                    }
                }   
            }
                if(player1Turn) {
                    bullet -> x = player2 -> x + 10;
                    bullet -> y = player2 -> y + 10;
                    player1Turn = 0;
                    speedX = 15;
                    speedY = -10;
                } else {
                    bullet -> x = player1 -> x + 10;
                    bullet -> y = player1 -> y + 10;
                    player1Turn = 1;
                    speedX = 15;
                    speedY = -10;
                }
            bullet -> fired = 0;
        }
        if(player1Turn){
            bullet -> color = player1 -> color;
        } else {
            bullet -> color = player2 -> color;
        }
        bullet -> x += bullet -> speedX;
        bullet -> y += bullet -> speedY;
        if (bullet->fired){
            bullet -> speedY += 1;
        }
        if(player1Turn) {
            aim -> x = player1 -> x + speedX * 6;
            aim -> y = player1 -> y + speedY * 3;
        } else {
            aim -> x = player2 -> x - speedX * 6;
            aim -> y = player2 -> y + speedY * 3;
        }
        waitTimer();
            initFrame(frame, colors);
            if(player1Health > 0 && player2Health > 0){
                if(player1Turn){
                    drawString(frame, 100, 10, player1TurnString, 0xF000);
                } else {
                    drawString(frame, 100, 10, player2TurnString, 0x00F0);
                }
                drawGround(frame, ground);
                if(bullet -> y <= 0){
                    bullet -> color = 0x0000;
                }
                drawRect(frame, bullet);
                drawRect(frame, aim);
                drawRect(frame, player1);
                drawImage(frame, player1, tankRed.pixel_data);
                drawRect(frame, player2);
                drawImage(frame, player2, tankBlue.pixel_data);
                if(explosion -> type){
                    drawRect(frame, explosion);
                    if(explosion -> color == 0xF0F0){
                        explosion -> color = 0xFF00;
                    } else if(explosion -> color == 0xFF00){
                        explosion -> color = 0xF000;
                    } else if(explosion -> color == 0xF000){
                        explosion -> type = 0;
                    }
                }
                drawHealth(frame, player2 -> x + 5,100,player2Health);
                drawHealth(frame, player1 -> x + 5,100,player1Health);
            }
            if(player1Health <= 0 || player2Health <= 0){
                state = '3';
            }
            outputFrame(fd, frame);
        }
    }
    stopTimer();
    disableInput();
    close(fd);
    return 0;
}
