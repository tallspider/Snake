#include <stdlib.h>
#include <stdbool.h>

//file globals**********************************************************************************
volatile int pixel_buffer_start; // global variable

int apple_x, apple_y, snake_dx, snake_dy, snake_x, snake_y;	//positions
int snake_length = 1;
short int apple_colour = 0xF800; // red
short int snake_colour = 0x03E0; // dark green
int score = 0;
bool legal_move = false, run = false, eat = false;

//prototypes*************************************************************************************
void clear_screen();
void draw_borders();
void plot_pixel(int x, int y, short int line_color);
void draw_line(int x0, int x1, int y0, int y1, short int colour);
void wait_for_vsync();
void draw_box(int x, int y, short int colour);
void swap(int * one, int * two);
bool is_legal(int x, int y);
bool has_eaten(int x0, int y0, int x1, int y1);

//implementations**********************************************************************************
int main(void) {
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int * edge_ptr = (int *)0xFF20005C; // edgecapture register

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
    
    // initializing snake to be in the middle of the screen
    snake_x = 159;
    snake_y = 119;
    snake_dx = 0;
    snake_dy = 0;
    
    // initializing apple to be in the same position each time at start
    apple_x = 200;
    apple_y = 119;
    
	//start the game
	run = true;
	legal_move = true;
	
    while (run) {
    	// erase prev snake
    	// polled IO for KEYs to determine which direction the snake moves in
    	// check key value and change dy or dx as necessary
    	// KEY0 = right, KEY1 = left, KEY2 = down, KEY3 = up
		// insert function that changes direction snake moves in correctly
		// increment dy or dx
		draw_box(snake_x, snake_y, snake_colour);
		draw_box(apple_x, apple_y, apple_colour);
    
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

    return 0;

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

void draw_borders() {
	draw_line(0, 0, 319, 0, 0xFFFF);
	draw_line(0, 0, 0, 239, 0xFFFF);
	draw_line(319, 0, 319, 239, 0xFFFF);
	draw_line(0, 239, 319, 239, 0xFFFF);
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
