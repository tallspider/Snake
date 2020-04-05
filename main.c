#include <stdlib.h>
#include <stdbool.h>

//type declarations*****************************************************************************
struct snake_node{
	int rb;
	int cb;
	struct snake_node *prev;
};

struct snake_list{
	struct snake_node *head;
	struct snake_node *tail;
};

//file globals**********************************************************************************
volatile int pixel_buffer_start; // global variable

struct snake_list snake;
int apple_x, apple_y, snake_dx, snake_dy, snake_x, snake_y, snake_length;
short int apple_colour = 0xF800; // red
short int snake_colour = 0x03E0; // dark green
int score = 0;
bool legal_move = false, run = false, eat = false;
bool game_started;

//prototypes*************************************************************************************
void clear_screen();
void draw_borders();
void plot_pixel(int x, int y, short int line_color);
void draw_line(int x0, int x1, int y0, int y1, short int colour);
void wait_for_vsync();
void draw_box(int x, int y, short int colour);
void draw_block(int rb, int cb, short int color);
void swap(int * one, int * two);

void init_snake();
void draw_first_snake();
void free_snake();

bool is_legal(int x, int y);
bool has_eaten(int x0, int y0, int x1, int y1);

//implementations**********************************************************************************
int main(void) {
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * edge_ptr = (int *)0xFF20005C; // edgecapture register

	//background setup ----------------------------------------------------------------------------
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
                                        
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
	
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
	
    clear_screen(); 
	draw_borders();
	
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
	
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    clear_screen();
	draw_borders(); // draw game borders
    
	//game setup-----------------------------------------------------------------------------------
    
	// initializing snake to be in the middle of the screen
    init_snake();
	
    // initializing apple to be in the same position each time at start
    apple_x = 200;
    apple_y = 119;
    
	//enter the game
	run = true;				//will probably use a button to start
	legal_move = true;
	
	//have to draw first snake and apple on both canvases
	draw_first_snake();
	draw_box(apple_x, apple_y, apple_colour);
	
	wait_for_vsync();
	pixel_buffer_start = *(pixel_ctrl_ptr + 1);
	
	draw_first_snake();
	draw_box(apple_x, apple_y, apple_colour);
	
	while (run) {
    	// erase prev snake tail
    	// polled IO for KEYs to determine which direction the snake moves in
    	// check key value and change dy or dx as necessary
    	// KEY0 = right, KEY1 = left, KEY2 = down, KEY3 = up
		// insert function that changes direction snake moves in correctly
		// increment dy or dx
		//draw_box(snake_x, snake_y, snake_colour);
		
		snake_x += snake_dx;
		snake_y += snake_dy;
		
		// check if legal
		if (legal_move) {
			
			//legal_move = false;
			eat = has_eaten(snake_x, snake_y, apple_x, apple_y);
			
			if (eat) {
				eat = false;
				
				// re-initialization of apple in a random position if snake has 'eaten' apple
				do{
					apple_x = (rand() % 314) + 3; // range from 3 to 316
					apple_y = (rand() % 234) + 3; // range from 3 to 236
				} while (!is_legal(apple_x, apple_y));
				// MUST MAKE SURE APPLE POSITION IS NOT SNAKE POSITION
				
				draw_box(apple_x, apple_y, apple_colour);
				
				// update snake length
				snake_length++;
				
				// update score
				score++;
				
			}
			
		} else {    
			// else game over
			// game over screen
			wait_for_vsync();
			run = false;
			break;
		}
		
		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
	}
	free_snake();
    return 0;

}

void init_snake(){
	// snake_x = 159;
    // snake_y = 119;
    snake_dx = 0;
    snake_dy = 0;
	// snake_length = 1;
	
	snake.head = (struct snake_node *)malloc(sizeof(struct snake_node));
	snake.tail = snake.head;
	snake.head -> rb = 24;
	snake.head -> cb = 32;
	snake.head -> prev = NULL;
}

void draw_first_snake(){
	draw_block(snake.head -> rb, snake.head -> cb, snake_colour);
}

void free_snake(){
	struct snake_node *at = snake.tail;
	while(at -> prev != NULL){
		at = at -> prev;
		free(snake.tail);
		snake.tail = at;
	}
	free(at);
}

void plot_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void draw_line(int x0, int y0, int x1, int y1, short int colour) {
	bool is_steep = (abs(y1 - y0) > abs(x1 - x0));

	if (is_steep) {
		swap(&x0, &y0);
		swap(&x1, &y1);
	}

	if (x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}


	int dx = x1 - x0;
	int dy = abs(y1 - y0);
	int error = (dx / 2) * (-1);
        int y = y0;
	int y_step = 0;
        if (y0 < y1) y_step = 1;
	    else y_step = -1;

	for (int x = x0; x < (x1+1); x++) {
	    if (is_steep) plot_pixel(y, x, colour);
		else plot_pixel(x, y, colour);

		error += dy;
		if (error >= 0) {
			y = y + y_step;
			error = error - dx;
		}
	}
}

void clear_screen() {
   for (int x = 0; x < 320; x++) { // 319 columns
	   for (int y = 0; y < 240; y++) { // 239 rows
		   plot_pixel(x, y, 0x0); // black is #000000
	   }
   }

}

void wait_for_vsync() {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	register int status;
	
	*pixel_ctrl_ptr = 1; // start synchronization (write to buffer register)
	status = *(pixel_ctrl_ptr + 3);

	while ((status & 0x01) != 0) { // wait for S to be 0
		status = *(pixel_ctrl_ptr + 3);
	}
}

void swap(int * one, int * two) {
	int temp = *one;
	*one = *two;
	*two = temp;
}

void draw_box(int x, int y, short int colour) {
	for(int i = x - 2; i <= x + 2; i++){
		for(int j = y - 2; j <= y + 2; j++){
			plot_pixel(i, j, colour);
		}
	}
}

void draw_block(int rb, int cb, short int color){
	draw_box(cb * 5 + 2, rb * 5 + 2, color);
}

void draw_borders() {
	// draw_line(0, 0, 319, 0, 0xFFFF);
	// draw_line(0, 0, 0, 239, 0xFFFF);
	// draw_line(319, 0, 319, 239, 0xFFFF);
	// draw_line(0, 239, 319, 239, 0xFFFF);
	
	//2 blocks thick borders all around
	int r, c;
	//left 
	for(r = 0; r < 48; r++){
		draw_block(r, 0, snake_colour);
		draw_block(r, 1, snake_colour);
	}
	
	//right 
	for(r = 0; r < 48; r++){
		draw_block(r, 63, snake_colour);
		draw_block(r, 62, snake_colour);
	}
	
	//top
	for(c = 0; c < 64; c++){
		draw_block(0, c, snake_colour);
		draw_block(1, c, snake_colour);
	}
	
	//bottom
	for(c = 0; c < 64; c++){
		draw_block(47, c, snake_colour);
		draw_block(46, c, snake_colour);
	}
		
}

bool is_legal(int x, int y) {
	if (x < 1 || y < 1 || x > 318 || y > 238) return false;
	else return true;
}

bool has_eaten(int x0, int y0, int x1, int y1) {
	return ((x0 == x1) && (y0 == y1));
}

/*
Init snake
Init apple
Draw snake
Draw apple
while(run){
	Save position of snake tail
update snake by defined increments
	Check if snake position is legal
	if(legal){
		Draw background color on tail block
		(also on head block if we want snake to have distinct head)
		Draw new head block
		if(snake head on apple){
			Increase points
			Update snake to be 1 longer
			Init new apple
			Draw new apple
}
wait_for_vsync()
} else {
	Draw Game Over panel
	wait_for_vsync()
	Draw Game Over panel
	Exit loop
}
}
*/
