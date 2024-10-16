#include <form.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include <sqlite3.h>
#include <cjson/cJSON.h>

#define mvwprintw_center(w, l, str) mvwprintw((w), (l), 40 - strlen(str) / 2, (str))

#define new_label_field_overbuff(label, field, text, x, y, field_size) new_label_field((label), (field), (text), (x), (y), field_size, 1)

bool end_for_real = false;

WINDOW* header_window;
WINDOW* elephant_window;
WINDOW* rules_window;

WINDOW* form_window;
WINDOW* information;
WINDOW* inner_information;
WINDOW* answer;
WINDOW* inner_answer;

static char* trim_whitespaces(char *str)
{
	char *end;

	while (isspace(*str)) {
		str++;
	}

	if (*str == 0)
		return strdup(str);

	end = str + strnlen(str, 128) - 1;

	while (end > str && isspace(*end)) {
		end--;
	}

	*(end + 1) = '\0';

	return strdup(str);
}

void new_label_field(FIELD** label, FIELD** field, const char* label_text, int x, int y, int field_size, int overbuff)
{
	size_t label_text_len = strlen(label_text);
	
	(*label) = new_field(1, label_text_len, x, y, 0, 0);
	set_field_buffer(*label, 0, label_text);
	set_field_opts(*label, O_VISIBLE | O_PUBLIC | O_AUTOSKIP | O_STATIC);

	(*field) = new_field(1, field_size, x, y + label_text_len + 1, overbuff, 0);
	set_field_opts(*field, O_VISIBLE | O_PUBLIC | O_EDIT | O_ACTIVE | O_STATIC);
}

char* created_button_label(const char* label)
{
    size_t len = strlen(label);
	size_t new_len = (len + 3) * 3;
    char* out = malloc(sizeof(char) * new_len);
    
    out[0] = '+';
    for (size_t i = 0; i < len; i += 1) {
        out[i + 1] = '-';
    }
    out[len + 1] = '+';
    out[len + 2] = '\n';
    out[len + 3] = '|';
    strcpy(out + len + 4, label);
    out[(len + 2) * 2] = '|';
    out[(len + 2) * 2 + 1] = '\n';
    strncpy(out + (len + 2) * 2 + 2, out, len + 2);
	out[new_len - 1] = '\0';
    return out;
}

FIELD* new_button(FIELD** field, const char* label, char** new_label, int x, int y)
{
	size_t label_len = strlen(label);
	*new_label = created_button_label(label);

	(*field) = new_field(3, label_len + 3 , x, y, 0, 0);
	set_field_opts((*field), O_VISIBLE | O_PUBLIC | O_ACTIVE | O_STATIC);
	set_field_buffer((*field), 0, *new_label);
	return (*field);
}

bool is_in_array(int to_check, const int* array, size_t array_len)
{
	for (size_t i = 0; i < array_len; i += 1) {
		if (to_check == array[i]) {
			return true;
		}
	}
	return false;
}

int multi_line_popup(const char* strs[], const int* exit_keys, size_t  exit_keys_len)
{
	size_t longuest_size = strlen(strs[0]);
	size_t nb_str = 0;
	int ch;

	for (size_t i = 0; strs[i] != NULL; i += 1) {
		size_t size = strlen(strs[i]);
		longuest_size = size > longuest_size ? size : longuest_size;
		nb_str += 1;
	}
	WINDOW* popup = newwin(2 + nb_str, longuest_size + 2, 12 - nb_str / 2, 40 - longuest_size / 2);
	wborder(popup, '|', '|', '-', '-', '+', '+', '+', '+');
	for (size_t i = 0; strs[i] != NULL; i += 1) {
		mvwprintw(popup, 1 + i, 1, "%s", strs[i]);
	}
	wrefresh(popup);
	if (exit_keys != NULL) {
		do {
			ch = getch();
		} while (!is_in_array(ch, exit_keys, exit_keys_len));
	}
	else {
		ch = getch();
	}
	delwin(popup);
	return ch;
}

bool handle_form(FORM* info_form, FIELD** info_fields, FORM* answer_form, FIELD** answer_fields, sqlite3* db)
{
	form_driver(info_form, REQ_VALIDATION);
	form_driver(answer_form, REQ_VALIDATION);
	char* name = trim_whitespaces(field_buffer(info_fields[1], 0));
	char* mail = trim_whitespaces(field_buffer(info_fields[3], 0));
	char* lastname = trim_whitespaces(field_buffer(info_fields[5], 0));
	char* phone = trim_whitespaces(field_buffer(info_fields[7], 0));
	char* answers;
	size_t nb_answers = 0;
	cJSON* array = cJSON_CreateArray();


	for (size_t i = 0; answer_fields[i] != NULL; i += 1) {
		if ((field_opts(answer_fields[i]) & O_ACTIVE) && answer_fields[i + 1] != NULL) {
			char* answer_str = trim_whitespaces(field_buffer(answer_fields[i], 0));
			if (strlen(answer_str) != 0) {
				nb_answers += 1;
			}
			cJSON* answer = cJSON_CreateString(answer_str);
			cJSON_AddItemToArray(array, answer);
			free(answer_str);
		}
	}
	answers = cJSON_PrintUnformatted(array);
	cJSON_Delete(array);
	char fullname_fmt[512];
	char mail_fmt[512];
	char phone_fmt[512];

	sprintf(fullname_fmt, " Vous etes %s %s ", name, lastname);
	sprintf(mail_fmt, " Mail: %s ", mail);
	sprintf(phone_fmt, " Telephone: %s ", phone);
	const char* strs[] = {
		"",
		fullname_fmt,
		mail_fmt,
		phone_fmt,
		" Est-ce que vous voulez validez vos reponses ? ",
		" (Appuyer sur \"O\" pour valider ou sur \"N\" pour revenir en arriere) ",
		"",
		NULL
	};
	const int exit_keys[] = {'o', 'n'};
	if (multi_line_popup(strs, exit_keys, 2) == 'n') {
		free(name);
		free(mail);
		free(lastname);
		free(phone);
		free(answers);
		return false;
	}
	sqlite3_stmt *st;
	sqlite3_prepare(db, "\
	        INSERT INTO Info\
            (name, mail, lastname, phone, answer)\
        		VALUES\
            (?, ?, ?, ?, ?);\
			", -1, &st, NULL);

	sqlite3_bind_text(st, 1, name, strlen(name), free);
	sqlite3_bind_text(st, 2, mail, strlen(mail), free);
	sqlite3_bind_text(st, 3, lastname, strlen(lastname), free);
	sqlite3_bind_text(st, 4, phone, strlen(phone), free);
	sqlite3_bind_text(st, 5, answers, strlen(answers), free);

	if (sqlite3_step(st) == SQLITE_ERROR) {
		const char* message[] = {
			" Vous avez deja participer ! ",
			" Une seule participation par personne !",
			NULL
		};
		multi_line_popup(message, NULL, 0);
	}
	sqlite3_finalize(st);
	return true;
}

int main() {
	bool end_loop = false;

	FIELD** info_fields = malloc(sizeof(FIELD*) * 9);
	FORM* info_form;
	
	FIELD** answer_fields = malloc(sizeof(FIELD*) * 22);
	FIELD* button;
	FORM* answer_form;

	FORM* selected_form = NULL;
	
	char* new_label;
	int ch;
	sqlite3 *db;

	sqlite3_open("database.db", &db);
	sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Info\
      (\
        name VARCHAR NOT NULL,\
        mail VARCHAR NOT NULL UNIQUE,\
        lastname VARCHAR NOT NULL,\
        phone VARCHAR NOT NULL,\
        answer TEXT NOT NULL\
      );\
    ", NULL, NULL, NULL);
start_over:
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	header_window = newwin(10, 80, 0, 0);

	elephant_window = derwin(header_window, 7, 16, 1, 3);
	rules_window = derwin(header_window, 8, 60, 1, 18);

	form_window = newwin(16, 80, 9, 0);

	information = derwin(form_window, 5, 80, 0, 0);
	inner_information = derwin(information, 3, 78, 1, 1);

	answer = derwin(form_window, 11, 80, 4, 0);
	inner_answer = derwin(answer, 9, 78, 1, 1);

	new_label_field_overbuff(&info_fields[0], &info_fields[1], "Prenom:", 0, 4, 19);
	new_label_field_overbuff(&info_fields[2], &info_fields[3], "Mail:", 2, 4, 21);
	new_label_field_overbuff(&info_fields[4], &info_fields[5], "Nom de famille:", 0, 38, 20);
	new_label_field_overbuff(&info_fields[6], &info_fields[7], "Numero de telephone:", 2, 38, 15);
	info_fields[8] = NULL;

	info_form = new_form(info_fields);
	set_form_win(info_form, information);
	set_form_sub(info_form, inner_information);

	new_label_field_overbuff(&answer_fields[0], &answer_fields[1], "Reponse #1:", 1, 4, 10);
	new_label_field_overbuff(&answer_fields[2], &answer_fields[3], "Reponse #2:", 3, 4, 10);
	new_label_field_overbuff(&answer_fields[4], &answer_fields[5], "Reponse #3:", 5, 4, 10);
	new_label_field_overbuff(&answer_fields[6], &answer_fields[7], "Reponse #4:", 7, 4, 10);

	new_label_field_overbuff(&answer_fields[8], &answer_fields[9], "Reponse #5:", 1, 29, 10);
	new_label_field_overbuff(&answer_fields[10], &answer_fields[11], "Reponse #6:", 3, 29, 10);
	new_label_field_overbuff(&answer_fields[12], &answer_fields[13], "Reponse #7:", 5, 29, 10);
	new_label_field_overbuff(&answer_fields[14], &answer_fields[15], "Reponse #8:", 7, 29, 10);

	new_label_field_overbuff(&answer_fields[16], &answer_fields[17], "Reponse #9:", 1, 54, 10);
	new_label_field_overbuff(&answer_fields[18], &answer_fields[19], "Reponse #10:", 3, 54, 9);

	button = new_button(&answer_fields[20], " Envoyer ", &new_label, 5, 55);

	answer_fields[21] = NULL;

	answer_form = new_form(answer_fields);
	set_form_win(answer_form, answer);
	set_form_sub(answer_form, inner_answer);

redraw:
	wborder(header_window, '|', '|', '-', '-', '+', '+', '+', '+');
	wborder(information, '|', '|', '-', '-', '+', '+', '+', '+');
	wborder(answer, '|', '|', '-', '-', '+', '+', '+', '+');
	refresh();
	mvwprintw_center(header_window, 0, " ELEPHANT technologies ");
	wrefresh(header_window);
	mvwprintw(elephant_window, 0, 0, "   _    _       ");
	mvwprintw(elephant_window, 1, 0, "  /=\\\"\"/=\\   ");
	mvwprintw(elephant_window, 2, 0, " (=(0_0 |=)__   ");
	mvwprintw(elephant_window, 3, 0, "  \\_\\ _/_/   )  ");
	mvwprintw(elephant_window, 4, 0, "    /_/   _  /\\ ");
	mvwprintw(elephant_window, 5, 0, "   |/ |\\ || |   ");
	mvwprintw(elephant_window, 6, 0, "      ~ ~  ~    ");
	wrefresh(elephant_window);
	mvwprintw(rules_window, 0, 3, "Pouvez-vous retrouver les 10 elements qui se cachent");
	mvwprintw(rules_window, 1, 3, "dans notre fresque ?");
	mvwprintw(rules_window, 3, 3, "Tentez de remporter des places pour un escape game");
	mvwprintw(rules_window, 4, 3, "d'horreur en trouvant un maximum de bonnes reponses !");
	mvwprintw(rules_window, 6, 3, "Chaque jour son gagnant et un tirage au sort sera");
	mvwprintw(rules_window, 7, 3, "effectue apres le DevFest en cas d'egalite.");
	wrefresh(rules_window);
	wrefresh(form_window);

	mvwprintw_center(answer, 0, " Reponses ");
	post_form(answer_form);
	wrefresh(inner_answer);
	wrefresh(answer);

	mvwprintw_center(information, 0, " Informations ");
	post_form(info_form);
	wrefresh(inner_information);
	wrefresh(information);

	selected_form = info_form;

	while (!end_loop)
	{
		ch = getch();
		if (ch == 27 /* ESC */) {
			ch = getch();

			if (ch == '[') {
				ch = getch();

				switch (ch)
				{
				case 'A':
					ch = KEY_UP;
					break;
				case 'B':
					ch = KEY_DOWN;
					break;
				case 'C':
					ch = KEY_RIGHT;
					break;
				case 'D':
					ch = KEY_LEFT;
					break;
				}
			}
		}

		switch (ch) {
			case KEY_DOWN:
				form_driver(selected_form, REQ_NEXT_FIELD);
				form_driver(selected_form, REQ_END_LINE);
				break;

			case KEY_UP:
				form_driver(selected_form, REQ_PREV_FIELD);
				form_driver(selected_form, REQ_END_LINE);
				break;

			case KEY_LEFT:
				form_driver(selected_form, REQ_PREV_CHAR);
				break;

			case KEY_RIGHT:
				form_driver(selected_form, REQ_NEXT_CHAR);
				break;

			// Delete the char before cursor
			case KEY_BACKSPACE:
			case 127:
				form_driver(selected_form, REQ_DEL_PREV);
				break;

			// Delete the char under the cursor
			case KEY_DC:
				form_driver(selected_form, REQ_DEL_CHAR);
				break;

			case '\t':
				selected_form = selected_form == info_form ? answer_form : info_form;
				break;

			case '\n':
				if (selected_form->current == button) {
					if ((end_loop = handle_form(info_form, info_fields, answer_form, answer_fields, db))) {
						for (size_t i = 0; info_fields[i] != NULL; i += 1) {
							if (field_opts(info_fields[i]) & O_ACTIVE) {
								set_field_buffer(info_fields[i], 0, "");
							}
						}
						for (size_t i = 0; answer_fields[i] != NULL; i += 1) {
							if (field_opts(answer_fields[i]) & O_ACTIVE) {
								set_field_buffer(answer_fields[i], 0, "");
							}
						}
						clear();
					}
					else {
						clear();
						goto redraw;
					}
				}
				break;

			default:
				form_driver(selected_form, ch);
				break;
		}
		wrefresh(form_sub(selected_form));
	}

	unpost_form(answer_form);
	free_form(answer_form);
	for (size_t i = 0; answer_fields[i] != NULL; i += 1) {
		free_field(answer_fields[i]);
	}

	unpost_form(info_form);
	free_form(info_form);
	for (size_t i = 0; info_fields[i] != NULL; i += 1) {
		free_field(info_fields[i]);
	}
	delwin(inner_answer);
	delwin(answer);
	delwin(inner_information);
	delwin(information);
	delwin(form_window);
	delwin(rules_window);
	delwin(elephant_window);
	delwin(header_window);
	endwin();
	free(new_label);
	end_loop = false;
	if (!end_for_real) {
		goto start_over;
	}
	free(info_fields);
	free(answer_fields);
	sqlite3_close(db);
}
