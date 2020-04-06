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
volatile int * pixel_ctrl_ptr;
volatile int * edge_ptr;

struct snake_list snake;
struct snake_node *prev_tail; 		//need this to separate drawing from updating snake location

int apple_rb, apple_cb, snake_drb, snake_dcb, snake_x, snake_y, snake_length;
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
void delay();

void check_direction_change();

void init_snake();
void draw_first_snake();
void move_snake();
void update_snake_drawing();
void lengthen_snake();
void draw_longer_snake();
void free_snake();

void init_apple();
void draw_apple();

bool is_legal(int x, int y);
bool check_move_legal();
bool will_eat_apple();

//implementations**********************************************************************************
int main(void) {
    pixel_ctrl_ptr = (int *)0xFF203020;
    edge_ptr = (int *)0xFF20005C; // edgecapture register
	
	//background setup ----------------------------------------------------------------------------
    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
                                        
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
	
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
	
    clear_screen(); 
	draw_borders();
	
    // /* set back pixel buffer to start of SDRAM memory */
    // *(pixel_ctrl_ptr + 1) = 0xC0000000;
	
	// pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    // clear_screen();
	// draw_borders(); // draw game borders
    
	//game setup-----------------------------------------------------------------------------------
    
	// initializing snake to be in the middle of the screen
    init_snake();
	
    // initializing apple to be in the same position each time at start
    init_apple();
    
	//enter the game
	run = true;				//will probably use a button to start
	legal_move = true;
	
	//have to draw first snake and apple on both canvases
	draw_first_snake();
	draw_apple();
	
	// wait_for_vsync();
	// pixel_buffer_start = *(pixel_ctrl_ptr + 1);
	
	// draw_first_snake();
	// draw_apple();
	
	while (run) {
		// erase prev snake tail
    	// polled IO for KEYs to determine which direction the snake moves in
    	// check key value and change dy or dx as necessary
    	// KEY0 = right, KEY1 = left, KEY2 = down, KEY3 = up
		// insert function that changes direction snake moves in correctly
		// increment dy or dx
		// draw_box(snake_x, snake_y, snake_colour);
		delay();
		// check edge capture first 
		check_direction_change();
		
		// check if legal
		if (check_move_legal()) {
			
			if (will_eat_apple()) {
				
				//apple is now part of the snake
				lengthen_snake();
				
				//draw changes to the snake
				draw_longer_snake();
				
				// wait_for_vsync();
				// pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				
				// draw_longer_snake();
				
				// re-initialization of apple in a random position if snake has 'eaten' apple
				do{
					apple_cb = (rand() % 60) + 2; // range from 2 to 61
					apple_rb = (rand() % 44) + 2; // range from 2 to 45
				} while (!is_legal(apple_rb, apple_cb));	//as long as apple is not on snake
				
				//draw_changes to the apple
				draw_apple();
				
				//draw on back canvas too
				// wait_for_vsync();
				// pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				
				// draw_apple();
				
				// update score
				score++;
				
			} else {
				
				move_snake();
				
				//draw on this canvas
				update_snake_drawing();
				
				//draw on back canvas too
				// wait_for_vsync();
				// pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
				
				// update_snake_drawing();
			}
			
		} else {    
			// else game over
			// game over screen
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
    snake_drb = 0;
    snake_dcb = 1;
	// snake_length = 1;
	
	snake.head = (struct snake_node *)malloc(sizeof(struct snake_node));
	snake.tail = snake.head;
	snake.head -> rb = 24;
	snake.head -> cb = 32;
	snake.head -> prev = NULL;
	
	prev_tail = NULL;
}

void init_apple(){
	apple_rb = 24;
	apple_cb = 48;
}

void draw_first_snake(){
	draw_block(snake.head -> rb, snake.head -> cb, snake_colour);
}

void draw_apple(){
	draw_block(apple_rb, apple_cb, apple_colour);
}

void move_snake(){
	//make new snake node for head, take off tail
	struct snake_node *new_head = (struct snake_node *)malloc(sizeof(struct snake_node));
	new_head -> rb = snake.head -> rb + snake_drb;
	new_head -> cb = snake.head -> cb + snake_dcb;
	new_head -> prev = NULL;
	
	snake.head -> prev = new_head;
	snake.head = snake.head -> prev;
	
	//take off the old tail
	if(prev_tail != NULL) free(prev_tail);
	prev_tail = snake.tail;
	snake.tail = snake.tail -> prev;
}

void lengthen_snake(){
	struct snake_node *new_head = (struct snake_node *)malloc(sizeof(struct snake_node));
	new_head -> rb = apple_rb;
	new_head -> cb = apple_cb;
	new_head -> prev = NULL;
	
	snake.head -> prev = new_head;
	snake.head = snake.head -> prev;
}

void draw_longer_snake(){
	draw_block(apple_rb, apple_cb, snake_colour);
}

void update_snake_drawing(){
	if(prev_tail != NULL)
		draw_block(prev_tail -> rb, prev_tail -> cb, 0);
	
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
		draw_block(r, 0, 0xFFFF);
		draw_block(r, 1, 0xFFFF);
	}
	
	//right 
	for(r = 0; r < 48; r++){
		draw_block(r, 63, 0xFFFF);
		draw_block(r, 62, 0xFFFF);
	}
	
	//top
	for(c = 0; c < 64; c++){
		draw_block(0, c, 0xFFFF);
		draw_block(1, c, 0xFFFF);
	}
	
	//bottom
	for(c = 0; c < 64; c++){
		draw_block(47, c, 0xFFFF);
		draw_block(46, c, 0xFFFF);
	}
		
}

void check_direction_change(){
	if((*edge_ptr) & 0xF){
			
		if((*edge_ptr) & 0x8){		//up
			
			if(snake_drb != 1){		//cannot reverse
				snake_drb = -1;
				snake_dcb = 0;
			}
		} else if((*edge_ptr) & 0x4){	//down
			
			if(snake_drb != -1){
				snake_drb = 1;
				snake_dcb = 0;
			}
		} else if((*edge_ptr) & 0x2){	//left 
			
			if(snake_dcb != 1){
				snake_drb = 0;
				snake_dcb = -1;
			}
		} else {
			
			if(snake_dcb != -1){
				snake_drb = 0;
				snake_dcb = 1;
			}
		}
		
		//reset edge capture
		*edge_ptr = 0xF;
	}
}

bool check_move_legal(){
	int next_rb = snake.head -> rb + snake_drb;
	int next_cb = snake.head -> cb + snake_dcb;
	
	//first check if they are both in range
	if(next_rb < 2 || next_rb > 45 || next_cb < 2 || next_cb > 61) return false;
	
	//next check if snake will eat itself
	struct snake_node *temp = snake.tail;
	while(temp != NULL){
		if(next_rb == temp -> rb && next_cb == temp -> cb) return false;
		temp = temp -> prev;
	}
	
	return true;
}

bool is_legal(int apple_rb, int apple_cb) {
	//check if the apple is on the snake
	struct snake_node *temp = snake.tail;
	while(temp != NULL){
		if(temp -> rb == apple_rb && temp -> cb == apple_cb) return false;
		temp = temp -> prev;
	}
	return true;
}

bool will_eat_apple() {
	int next_rb = snake.head -> rb + snake_drb;
	int next_cb = snake.head -> cb + snake_dcb;
	return next_rb == apple_rb && next_cb == apple_cb;
}

void delay() {
 	// A9 private timer address values
 	volatile int *timer = 0xFFFEC600;
 	volatile int *timer_ae = 0xFFFEC608;
 	volatile int *timer_f = 0xFFFEC60C;
	
 	// load delay of 0.25s
 	*timer = 50000000;
	
 	// set A & E bits
 	*timer_ae = 0b11;
	
	// keep polling until F = 0
 	while (*timer_f != 0x1) {
 		continue;
 	} 
	
	*timer_f = 0x1; // reset F bit
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
