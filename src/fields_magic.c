/*
 * Simple ncurses form example with fields that actually behaves like fields.
 *
 * How to run:
 *	gcc -Wall -Werror -g -pedantic -o test fields_magic.c -lform -lncurses
 */

#include "fields_magic.h"

#include <ncurses.h>
#include <form.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static FORM *form;
static FIELD **fields;
static WINDOW *win_body, *win_form;

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
	printw("Form data saved");

	refresh();
	pos_form_cursor(form);
}

static void driver(int ch, struct formdata* _formdata)
{
	switch (ch) {
		case KEY_F(2):
			keyfun_save(_formdata);
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

		default:
			form_driver(form, ch);
			break;
	}

	wrefresh(win_form);
}

/* Populate fields with the given parameters */
int populateFields(struct formdata* _formdata,
		   unsigned int _numfields)
{
	assert(_formdata);

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
	}

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

	mvwprintw(win_body, 1, 2,
		  "Press F1 to quit and F2 to save fields content");
	fields = malloc((_numfields * 2 + 1) * sizeof(FIELD*));
	if (fields == NULL) {
		/* return error if malloc fails */
		printf("Out of memory\n");
		return 1;
	}

	/* Create required fields */
	if (populateFields(_formdata, _numfields) != 0)
		return 1;

	fields[_numfields * 2] = NULL;

	form = new_form(fields);
	assert(form != NULL);
	set_form_win(form, win_form);
	set_form_sub(form, derwin(win_form, 18, 76, 1, 1));
	post_form(form);

	refresh();
	wrefresh(win_body);
	wrefresh(win_form);

	/* Generate form and monitor key presses */
	while ((ch = getch()) != KEY_F(1))
		driver(ch, _formdata);

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

/* Example main:
int main()
{
	// Test set of labels for form
	struct formdata smeg[] = {
		{
			.name = "Location",
			.data = ""
		},

		{
			.name = "Subject",
			.data = ""
		},

		{
			.name = "Date",
			.data = ""
		},

		{
			.name = "Camera",
			.data = ""
		}
	};

	// Construct form from struct
	if (buildForm(smeg, sizeof(smeg)/sizeof(struct formdata)) != 0)
		return 1;

	// If successfull, report results
	for (uint32_t i = 0; i < sizeof(smeg)/sizeof(struct formdata); ++i) {
		printf("%s: %s\n", smeg[i].name, smeg[i].data);
	}

	return 0;
}
*/
