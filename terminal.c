// Copyright Xunillen. (https://github.com/Xunillen). Korišteno u pico projekt za pisanje po random mjestima.
// MIT licenca

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

enum KEYS
  {
   ARROW_UP = 1024,
   ARROW_DOWN,
   ARROW_LEFT,
   ARROW_RIGHT
  };
enum CURSOR_DEFAULT
  {
   TOP_LEFT,
   TOP_RIGHT,
   BOTTOM_LEFT,
   BOTTOM_RIGHT
  };
enum DIRECTION
  {
   UP,
   DOWN,
   LEFT,
   RIGHT
  };

static struct termios orig_termios;

int
exit_raw_mode()
{
  if (!tcsetattr(STDIN_FILENO, TCIOFLUSH, &orig_termios))
    return 0;
  return 1;
}

int
enter_raw_mode()
{
  struct termios modf_termios;

  /*
   * Get current terminal attributes and store it to orig_termios.
   * modf_termios is variable that we modify for entering raw mode.
   */
  if (tcgetattr (STDOUT_FILENO, &modf_termios) != 0)
    return 1;
  orig_termios = modf_termios;

  /* 
   * tcflag_t c_iflag    - input modes
   * tcflag_t c_oflag    - output modes
   * tcflag_t c_cflag    - control modes
   * tcflag_t c_lflag    - local modes
   * cc_t     c_cc[NCCS] - special characters
   */
  
  /*
   * c_iflag - input flags:
   *
   * IGNBRK  - Ignore break condition.
   * PARMARK - If neither PARMRK nor IGNPAR is setm read characters with parity error
   *           or framing error as \0 (default is \377 \0).
   * ISTRIP  - Strip off eight bit (no error checking).
   * INLCR   - Translate NL to CR on input.
   * IGNCR   - Ignore carrige return on input.
   * ICRNL   - Translate carrige return to new line on input.
   * IXON    - Enable XON/XOFF flow control on input.
   */
  modf_termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR
			    | IGNCR | ICRNL | IXON);
  /*
   * c_oflag - output flag:
   *
   * OPOST - Enable implementation-defined output processing.
   */
  modf_termios.c_oflag &= ~OPOST;

  /*
   * c_lflag - local flags:
   *
   * ECHO   - Echo input characters.
   * ECHONL - Echo NL characters.
   * ICANON - Enable canonical mode
   * ISIG   - Generate correspoding signal on input of INTR, QUIT, SUSP, or DSUSP.
   * IEXTEN - Enable implementation-defined input processing.
   */
  modf_termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  /*
   * c_cflag - control flags: 
   *
   * CSIZE  - Character size mask.
   * PARENB - Enable parity generation on output and parity checking for input.
   */
  modf_termios.c_cflag &= ~(CSIZE | PARENB);
  modf_termios.c_cflag |= CS8;

  // Set new termios and flush input and output
  if (tcsetattr (STDOUT_FILENO, TCIOFLUSH, &modf_termios) != 0)
    return 1;
  return 0;
}


/**Screen controls**/
/*
 * Clear screen.
 */
int
clear_screen()
{
  if (write(STDOUT_FILENO, "\x1b[2J", 4) != 4)
    return 1;
  return 0;
}


/*
 * Gets current cursor possition.
 */
int
get_cursor_position(int *x, int *y)
{
  char buff[32];
  unsigned int i = 0;
  
  if (write (STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return 1;
  while (read(STDIN_FILENO, &buff[i], 1) == 1)
    {
      if (buff[i] == 'R')
	break;
      i++;
    }
  buff[i] = '\0';
  if (sscanf (&buff[2], "%d;%d", y, x) != 2)
    return 1;

  return 0;
}

/*
 * Moves cursor in speciffic direction with definied count.
 */
int
move_cursor (enum KEYS arrow_key, int count)
{
  char cntrl_seq[4];

  // Build default cursor control sequence
  cntrl_seq[0] = '\x1b';
  cntrl_seq[1] = '[';
  cntrl_seq[2] = '1';

  switch (arrow_key)
    {
    case ARROW_UP:
      cntrl_seq[3] = 'A';
      break;
    case ARROW_DOWN:
      cntrl_seq[3] = 'B';
      break;
    case ARROW_RIGHT:
      cntrl_seq[3] = 'C';
      break;
    case ARROW_LEFT:
      cntrl_seq[3] = 'D';
      break;
    }
  for (int i = 0; i < count; i++)
    if (write(STDOUT_FILENO, cntrl_seq, 4) != 4)
      return 1;
  return 0;
}

/*
 * Move cursor position to one of predefinied positions.
 */
int
move_cursor_default (enum CURSOR_DEFAULT cur_def)
{
  /*
   * Not implemented.
   * Edit: donekle implementirano na los nacin, i bez povratka vrijednosti.
   */
  switch (cur_def)
    {
    case TOP_LEFT:
      if (write(STDOUT_FILENO, "\x1b[0;0f", 6) != 6)
        return 1;
      break;
    case TOP_RIGHT:
	move_cursor (ARROW_UP, 999);
	move_cursor (ARROW_RIGHT, 999);
      break;
    case BOTTOM_LEFT:
	move_cursor (ARROW_DOWN, 999);
	move_cursor (ARROW_LEFT, 999);
      break;
    case BOTTOM_RIGHT:
	move_cursor (ARROW_DOWN, 999);
	move_cursor (ARROW_RIGHT, 999);
      break;
    }
  return 0;
}


/*
 * Get terminal windows size.
 * It uses VT100 to move cursor to bottom edge of terminal,
 * and then reads cursor postion with "Query Cursor position" sequence.
 */
int
get_terminal_size (int *widht, int *height)
{
  int orig_x, orig_y;
  struct winsize w_size;
  
  // 1st solution, using ioctl(), provided by
  // sys/ioctl.h library. This function does not guarante
  // that it will work on all OS.
  if (ioctl (0, TIOCGWINSZ, &w_size) == 0 || w_size.ws_col != 0)
    {
      *height = w_size.ws_row;
      *widht = w_size.ws_col;
      return 0;
    }
  // 2nd solution, move cursor to position 999x999, and get size
  // save current cursor positon
  get_cursor_position (&orig_x, &orig_y);
  
  if (move_cursor (ARROW_DOWN, 999)
      && move_cursor (ARROW_RIGHT, 999) != 0)
    return 1;
  if (get_cursor_position (widht, height) != 0)
    return 1;
  
  // Restore cursor position
  if (move_cursor_default (TOP_LEFT) != 0
      && move_cursor (ARROW_RIGHT, --orig_x) != 0
      && move_cursor (ARROW_DOWN, --orig_y) != 0)
    return -1;
  
  return 0;
}

/* Draw functions */
int
draw_line (int start_x, int start_y,
	   int lenght, enum DIRECTION direction)
{
  int temp_y = start_y; // Variable for
  int temp_x = start_x;

  move_cursor_default (TOP_LEFT);
  move_cursor (ARROW_RIGHT, start_x);
  move_cursor (ARROW_DOWN, start_y);

  switch (direction)
    {
    case UP:
      for (int i = 0; i < lenght; i++)
	{
	  move_cursor_default (TOP_LEFT);
	  move_cursor (ARROW_RIGHT, start_x);
	  move_cursor (ARROW_DOWN, temp_y);

	  if (write(STDOUT_FILENO, "|", 1) != 1)
	    return 1;
	  temp_y--;
	}
      break;
    case DOWN:
      for (int i = 0; i < lenght; i++)
	{
	  move_cursor_default (TOP_LEFT);
	  move_cursor (ARROW_RIGHT, start_x);
	  move_cursor (ARROW_DOWN, temp_y);

	  if (write(STDOUT_FILENO, "|", 1) != 1)
	    return 1;
	  temp_y++;
	}
      break;
    case LEFT:
      for (int i = 0; i < lenght; i++)
	if (write(STDOUT_FILENO, "-", 1) != 1)
	  return 1;
      break;
    case RIGHT:
      for (int i = 0; i < lenght; i++)
	{
	  move_cursor_default (TOP_LEFT);
	  move_cursor (ARROW_RIGHT, temp_x);
	  move_cursor (ARROW_DOWN, temp_y);
	  
	  if (write(STDOUT_FILENO, "-", 1) != 1)
	    return 1;
	  temp_x--;
	}
      break;
    }
  return 0;
}

int
main()
{
  int x, y, x_temp, y_temp, widht, height;
  char c;
  char message[] = "This is basic cursor control test.\r\n"\
    "w or UP arrow - move cursor up\r\n"\
    "s or DOWN arrow - move cursor down\r\n"\
    "a or Left arrow - move cursor left\r\n"\
    "d or RIGHT arrow - move cursor right";
  char out[36];
  enter_raw_mode();
  clear_screen();
  move_cursor_default(TOP_LEFT);
  clear_screen();
  write (STDOUT_FILENO, message, sizeof(message));
  get_terminal_size (&widht, &height);
  
  draw_line (widht/2, height/2, height/2, UP);
  draw_line (widht/2, height/2, height/2, DOWN);
  draw_line (widht/2, height/2, widht/2, LEFT);
  draw_line (widht/2, height/2, widht/2, RIGHT);
  move_cursor_default (TOP_LEFT);
  move_cursor (ARROW_DOWN, height/2);
  move_cursor (ARROW_RIGHT, widht/2); 

  while (read(STDIN_FILENO, &c, 1))
    {
      if (c == 'q')
	break;
      if (c == 'w')
	move_cursor (ARROW_UP, 1);
      if (c == 's')
	move_cursor (ARROW_DOWN, 1);
      if (c == 'd')
	move_cursor (ARROW_RIGHT, 1);
      if (c == 'a')
	move_cursor (ARROW_LEFT, 1);

      get_cursor_position (&x, &y);
      x_temp = x;
      y_temp = y;
      
      move_cursor_default (TOP_LEFT);
      sprintf (out, "Cursor position_x:%d position_y:%d  ", x, y);
      write (STDOUT_FILENO, out, sizeof(out));
      move_cursor_default (TOP_LEFT);
      move_cursor (ARROW_DOWN, --y_temp);
      move_cursor (ARROW_RIGHT, --x_temp);
      
    }
  get_cursor_position (&x, &y);
  printf ("\n\nx_position = %d, y_position = %d\n"\
	  "terminal_widht=%d, terminal_height=%d", x, y, widht, height);
  exit_raw_mode();
 return 0;
}
