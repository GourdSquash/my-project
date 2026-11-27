#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>

#define JTED_VERSION "0.0.1"

enum editorKey {
	MOVE_LEFT  = 1000,
	MOVE_RIGHT,
	MOVE_DOWN,
	MOVE_UP,
	PAGE_DOWN,
	PAGE_UP,
	GO_NORMAL,
	GO_INSERT,
	QUIT,
};

enum editorMode {
	NORMAL_MODE,
	INSERT_MODE,
};

/* data */
struct editorConfig {
	int cx, cy;
	int screenrows;
	int screencols;
	struct termios orig_termios;
	enum editorMode mode;
};
struct editorConfig E;
void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

/* terminal */
void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1){
		die("tcsetattr");
	}
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
		die("tcgetattr");
	}
	atexit(disableRawMode);
	struct termios raw = E.orig_termios;

	raw.c_lflag &= ~(ECHO | ICANON);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
		return -1;
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

int editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1){
		if (nread == -1 && errno != EAGAIN) die("read");
	}
	if (c == '\x1b'){
		char seq[3];
		if (read(STDIN_FILENO, &seq[0],1) != 1) return GO_NORMAL;
		if (read(STDIN_FILENO, &seq[1],1) != 1) return GO_NORMAL;

		if (seq[0] == '[') {
		      if (seq[1] >= '0' && seq[1] <= '9') {
			if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~') {
			  switch (seq[1]) {
			    case '5': return PAGE_UP;
			    case '6': return PAGE_DOWN;
			  }
			}
		      
		} else {
		}
		}
		return GO_NORMAL;
	} else {
		if (E.mode == NORMAL_MODE) {switch (c) {
			case 'h':
				return MOVE_LEFT;
				break;
			case 'l':
				return MOVE_RIGHT;
				break;
			case 'j':
				return MOVE_DOWN;
				break;
			case 'k':
				return MOVE_UP;
				break;
			case 'i':
				return GO_INSERT;
				break;
		}}
		if (c == 'q') {
			return QUIT;
		}else {
	return c;
		}
	}
}
/* append buffer */
struct abuf {
	char *b;
	int len;
};
#define ABUF_INIT {NULL, 0}
void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

/* input */
void editorMoveCursor(int key){
	switch (key) {
		case MOVE_LEFT:
			if (E.cx != 0) E.cx--;
			break;
		case MOVE_RIGHT:
			if (E.cx < E.screencols) E.cx++;
			break;
		case MOVE_DOWN:
			if (E.cy < E.screenrows) E.cy++;
			break;
		case MOVE_UP:
			if (E.cy != 0) E.cy--;
			break;
	}
}
void editorProcessCommand(){
	int c = editorReadKey();
	switch (c) {
		case QUIT:
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		case MOVE_LEFT:
		case MOVE_RIGHT:
		case MOVE_DOWN:
		case MOVE_UP:
			editorMoveCursor(c);
			break;
		case PAGE_UP:
		case PAGE_DOWN:
			{
				int times = E.screenrows;
			while (times--)
				editorMoveCursor(c == PAGE_UP ? MOVE_UP : MOVE_DOWN);
			}
			break;
		case GO_INSERT:
			E.mode = INSERT_MODE;
			break;
		case GO_NORMAL:
			E.mode = NORMAL_MODE;
			break;
			
			
	}
}
/* output */
void editorDrawRows(struct abuf *ab) {
	int y;
	for (y=0;y<E.screenrows;y++){
		if (y == E.screenrows /3) {
			char welcome[80];
			int welcomelen = snprintf(welcome, sizeof(welcome),
					"jted -- version %s", JTED_VERSION);
			if (welcomelen > E.screencols) welcomelen = E.screencols;
			int padding = (E.screencols - welcomelen) / 2;
		       if (padding) {
			       abAppend(ab,"~",1);
			       padding--;
		       }	       
		       while (padding--) abAppend(ab, " ", 1);
			abAppend(ab, welcome, welcomelen);
	} else {

		abAppend(ab, "~", 1);
	}
		abAppend(ab, "\x1b[K", 3);
		if (y < E.screenrows -1){
			abAppend(ab, "\r\n", 2);
		}
	}
}
void editorRefreshScreen() {
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	editorDrawRows(&ab);

	char buf[32];
	/* if (E.cx < E.screencols) { */
	/* 	E.cx++; */
	/* } else E.cx = 0; */
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

/* init */
void initEditor() {
	E.cx = 0;
	E.cy = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){
	enableRawMode();
	initEditor();
	while (1) {
		editorRefreshScreen();
		editorProcessCommand();
	}
	return 0;
}
