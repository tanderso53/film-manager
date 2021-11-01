/*
 * Simple ncurses form example with fields that actually behaves like fields.
 *
 * How to run:
 *	gcc -Wall -Werror -g -pedantic -o test fields_magic.c -lform -lncurses
 */

#include "fields_magic.h"

#include <ncurses.h>
#include <form.h>
#include <menu.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define FM_MODE_MENU 0x01
#define FM_MODE_FORM 0x02

extern const char* const sys_errlist[];
extern const int sys_nerr;

/* NCURSES globals */
static WINDOW* win_body;
static int ncurses_mode = FM_MODE_FORM;

/* Form globals */
static FORM* form;
static FIELD** fields;
static WINDOW* win_form;
int fm_borked = 0; /* If non-zero, there has been an error */

/* Menu globals */
static MENU* menu;
static ITEM** items;
static WINDOW* win_menu;

/* Sample autocomplete strings for testing */
char* sample_ac_name[] = {
	"Sample 1",
	"Sample 2",
	"Sample 3",
	"Sample 4",
	"Sample 5"
};

char* sample_ac_desc[] = {
	"",
	"",
	"",
	"",
	""
};

/*
 * This is useful because ncurses fill fields blanks with spaces.
 */
static char* trim_whitespaces(char *str)
{
	char *end;

	assert(str);

	// trim leading space
	while(isspace(*str))
		str++;

	if(*str == 0) // all spaces?
		return str;

	// trim trailing space
	end = str + strnlen(str, 128) - 1;

	while(end > str && isspace(*end))
		end--;

	// write new null terminator
	*(end+1) = '\0';

	return str;
}

static void keyfun_save(struct formdata* _formdata)
{
	// Or the current field buffer won't be sync with what is displayed
	form_driver(form, REQ_NEXT_FIELD);
	form_driver(form, REQ_PREV_FIELD);
	move(2, 2);

	for (int i = 0; fields[i]; i = i + 2) {
		// Add data to form
		strcpy(_formdata[i / 2].data,
		       trim_whitespaces(field_buffer(fields[i + 1], 0)));
	}

	/* Inform the user that save was completed */
	printw("                                       ");
	move(2,2);
	printw("Form data saved");

	refresh();
	pos_form_cursor(form);
}

static void pagenum_update()
{
	move(2,2);
	printw("                                       ");
	move(2,2);
	printw("%d/%d", form_page(form) + 1, form->maxpage);
	refresh();
	pos_form_cursor(form);
}

/* Populate fields with the given parameters */
int populateFields(struct formdata* _formdata,
		   unsigned int _numfields)
{
	assert(_formdata);
	int winsize = 18;
	int pageno = 1;

	/* Create required fields */
	for (uint8_t i = 0; i < 2 * _numfields; i = i + 2) {
		/* Use two rows of film and set offset width */
		fields[i] = new_field(1, 10, i, 0, 0, 0);
		fields[i+1] = new_field(1, 40, i, 15, 0, 0);
		assert(fields[i] != NULL && fields[i+1] != NULL);

		/* Set initial field data */
		set_field_buffer(fields[i], 0, _formdata[i/2].name);
		set_field_buffer(fields[i + 1], 0, _formdata[i/2].data);

		/* Set skip for label and edit for entry field */
		set_field_opts(fields[i],
			       O_VISIBLE | O_PUBLIC | O_AUTOSKIP);
		set_field_opts(fields[i + 1],
			       O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE);

		/* Underline entry forms */
		set_field_back(fields[i + 1], A_UNDERLINE);

		/* Are these forms on a new page? */
		if (i > pageno * (winsize - 1)) {
			set_new_page(fields[i], true);
			pageno++;
		}

		if (pageno > 1) {
			uint16_t offset = i - (pageno - 1) * (winsize);

			if (move_field(fields[i], offset, 0) != E_OK) {
				assert(0);
				return 1;
			}

			if (move_field(fields[i + 1], offset, 15) != E_OK) {
				assert(0);
				return 1;
			}
		}
	}

	fields[_numfields * 2] = NULL;

	return 0;
}

int setFormGeometry()
{
	win_body = newwin(24, 80, 0, 0);
	assert(win_body != NULL);
	box(win_body, 0, 0);
	win_form = derwin(win_body, 20, 78, 3, 1);
	assert(win_form != NULL);
	box(win_form, 0, 0);
	return 0;
}

int initializeForm()
{
	form = new_form(fields);
	assert(form != NULL);
	set_form_win(form, win_form);
	set_form_sub(form, derwin(win_form, 18, 76, 1, 1));

	if (post_form(form) != E_OK)
		return 1;

	if (refresh() == ERR)
		return 1;
	if (wrefresh(win_body) == ERR)
		return 1;
	if (wrefresh(win_form) == ERR)
		return 1;

	return 0;
}

int initializeMenu(int _namesc, char* const* _namesv,
		   char * const* _descv)
{
	/* Generate list of items*/
	items = malloc((_namesc + 1) * sizeof(ITEM*));

	for (int i = 0; i < _namesc; ++i) {
		items[i] = new_item(_namesv[i], _descv[i]);

		if (!items[i])
			return 1;
	}

	items[_namesc] = NULL;

	/* Create the menu */
	menu = new_menu(items);
	assert(menu);

	/* Assign menu to windows */
	assert(win_body);
	win_menu = derwin(win_body, 20, 78, 3, 1);
	set_menu_win(menu, win_menu);
	set_menu_sub(menu, derwin(win_menu, 18, 76, 1, 1));

	/* Post menu and write to screen */
	assert(menu);
	unpost_form(form);
	post_menu(menu);

	if (refresh() == ERR)
		return 1;

	if (wrefresh(win_body) == ERR)
		return 1;

	if (wrefresh(win_menu) == ERR)
		return 1;

	ncurses_mode = ncurses_mode | FM_MODE_MENU;

	return 0;
}

int endMenu(char* result, int len)
{
	/* Replace menu window with form */
	unpost_menu(menu);
	post_form(form);

	if (refresh() == ERR)
		return 1;
	if (wrefresh(win_body) == ERR)
		return 1;
	if (wrefresh(win_form) == ERR)
		return 1;

	ncurses_mode = ncurses_mode & (~FM_MODE_MENU);

	/* Get result from menu */
	ITEM* curitem = current_item(menu);

	if (!curitem) {
		/* Empty string if no item selected */
		strncpy(result, "", len);
	}
	else {
		strncpy(result, item_name(curitem), len);
	}

	/* Free menu objects */
	free_menu(menu);

	for (int i = 0; items[i]; i++) {
		if (free_item(items[i]) != E_OK) {
			return 1;
		};
	}

	free(items);

	return 0;
}

static void driver(int ch, struct formdata* _formdata)
{
	switch (ch) {
	case KEY_F(2):
		keyfun_save(_formdata);
		break;

	case KEY_F(3):
		initializeMenu(5, sample_ac_name, sample_ac_desc);
		break;

	case KEY_DOWN:
	case 9:
		form_driver(form, REQ_NEXT_FIELD);
		form_driver(form, REQ_END_LINE);
		break;

	case KEY_UP:
		form_driver(form, REQ_PREV_FIELD);
		form_driver(form, REQ_END_LINE);
		break;

	case KEY_LEFT:
		form_driver(form, REQ_PREV_CHAR);
		break;

	case KEY_RIGHT:
		form_driver(form, REQ_NEXT_CHAR);
		break;

		// Delete the char before cursor
	case KEY_BACKSPACE:
	case 127:
		form_driver(form, REQ_DEL_PREV);
		break;

		// Delete the char under the cursor
	case KEY_DC:
		form_driver(form, REQ_DEL_CHAR);
		break;

	case KEY_NPAGE:
		form_driver(form, REQ_NEXT_PAGE);
		/* set_form_page(form, form_page(form) + 1); */
		pagenum_update();
		break;

	case KEY_PPAGE:
		form_driver(form, REQ_PREV_PAGE);
		/* set_form_page(form, form_page(form) - 1); */
		pagenum_update();
		break;

	default:
		form_driver(form, ch);
		break;
	}

	if (wrefresh(win_form) == ERR)
		fm_borked = 1;
}

/* Menu driver, req continue if 0, break if 1, error on 2 */
int mdriver(int ch)
{
	int c;
	char buf[32];

	switch(ch) {
	case KEY_UP:
		c = REQ_PREV_ITEM;
		break;
	case KEY_DOWN:
		c = REQ_NEXT_ITEM;
		break;
	case '\n':
		endMenu(buf, 32);
		if (set_field_buffer(current_field(form), 0, buf) != E_OK) {
			fprintf(stderr,
				"Error saving menu: %s",
				strerror(errno));
			return 1;
		}
		wrefresh(win_form);
		return 0;
	default:
		return 0;
	}

	if (menu_driver(menu, c) != E_OK) {
		return 1;
	}

	wrefresh(win_menu);

	return 0;
}

int buildForm(struct formdata* _formdata, uint8_t _numfields)
{
	int ch;

	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	if (setFormGeometry() != 0) {
		return 1;
	}

	/* Instructions header */
	mvwprintw(win_body, 1, 2,
		  "Press F1 to quit and F2 to save fields content");
	mvwprintw(win_body, 1, 59,
		  "PAGE UP:   Prev Page");
	mvwprintw(win_body, 2, 59,
		  "PAGE DOWN: Next Page");

	fields = malloc((_numfields * 2 + 1) * sizeof(FIELD*));
	if (fields == NULL) {
		/* return error if malloc fails */
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	/* Create required fields */
	if (populateFields(_formdata, _numfields) != 0)
		return 1;

	/* Initialize form with given fields and windows */
	if (initializeForm() != 0)
		return 1;

	pagenum_update();

	/* Generate form and monitor key presses */
	while ((ch = getch()) != KEY_F(1)) {
		if ((ncurses_mode & FM_MODE_MENU) == FM_MODE_MENU)
			mdriver(ch);
		else
			driver(ch, _formdata);
	}

	/* Free all form information */
	unpost_form(form);
	free_form(form);
	for (uint8_t i = 0; i < 2 * _numfields; ++i)
		free_field(fields[i]);
	delwin(win_form);
	delwin(win_body);
	endwin();

	/* Free array of fields */
	free(fields);

	return 0;
}
