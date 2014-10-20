#include <ncurses/ncurses.h>

int main()
{
	initscr();
	cbreak();
	refresh();
	WINDOW *w = newwin(10,40,0,0);
	box(w,0,0);
	mvwprintw(w, 0, 1, "%s %d %d", "Hello ncurses world!", COLS, LINES);
	wrefresh(w);
	getch();
	endwin();

	return 0;
}
