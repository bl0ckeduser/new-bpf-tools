/*
 * Textdriven game where you can go outdoors
 * and then unfortunate things happen to you
 */

#include <stdio.h>

int tablen = 0;
char *tab[128];

void mktab(char *str)
{
	tab[tablen] = malloc(strlen(str) + 1);
	strcpy(tab[tablen], str);
	++tablen;
}

typedef struct game_state {
	char actions[32][64];
	int action_count;
	struct game_state *dest[32];
	char desc[256];
} gs_t;

struct game_state* mkstate(char* desc)
{
	struct game_state* sta = malloc(sizeof(struct game_state));
	strcpy(sta->desc, desc);
	sta->action_count = 0;
	return sta;
}

void add_action(struct game_state* gs, char *act, struct game_state *dst)
{
	strcpy (gs->actions[gs->action_count], act);
	gs->dest[gs->action_count] = dst;
	gs->action_count++;
}

main()
{
	struct game_state* curr_sta = 0x0;
	char buf[128];

/*0*/	mktab("You are in a dark room. You may :TURNON:1: the lights.");
/*1*/	mktab("You are in a lit room. You may :TURNOFF:0: the lights, or go :OUTSIDE:2: now that the door is visible.");
/*2*/	mktab("You are outside. It's quite fucking cold. You may :WALK:3: or go back :INSIDE:4: get your gloves");
/*3*/	mktab("You become very very cold and got hypothermia. :$:0:");
/*4*/	mktab("You are in a lit room. You may now pick up your gloves and :GTFO:5:");
/*5*/	mktab("Some asshole spat at your face and judged you, and it felt terrible. :$:0: ");

	gs_t* states[64];
	int i;
	char *ptr;
	char *p, *q;
	char tag[32];
	char id[32];
	int idno;
	char trim[128];
	int lev;
	int j;


	for (i = 0; i < tablen; ++i) {
		states[i] = mkstate("");
	}

	for (i = 0; i < tablen; ++i) {
		lev = 0;
		j = 0;
		for (ptr = tab[i]; *ptr; ++ptr) {
			if (*ptr == ':') {
				if (lev == 1)
					*p++ = 0;
				if (lev == 2)
					*q++ = 0;
				++lev;
				lev %= 3;
				if (lev == 0) {
					sscanf(id, "%d", &idno);
					add_action(states[i], tag, states[idno]);
				}
			} else if (lev != 2 && *ptr != '$')
				trim[j++] = *ptr;

			if (lev == 1 && *ptr != ':') {
				*p++ = *ptr;
			} else p = &tag[0];

			if (lev == 2 && *ptr != ':') {
				*q++ = *ptr;
			} else q = &id[0];
		}
		trim[j] = 0;
		strcpy(states[i]->desc, trim);
	}

	curr_sta = *states;

	for (i = 0; i < 5; ++i)
		puts("");

	printf("	WELCOME TO THE				\n");
	printf("	EXTRA MEGA PLUS TURBO CHARGED		\n");
	printf("	SUPER ULTRA UBER INCREDIBLE		\n");
	printf("	HIGH-TECH MINDBLOWING 			\n");
	printf("   ========================================== 	\n");
	printf("    INCREDIBLE TEXT RPG GAME THING 2013		\n");
	printf("   ========================================== 	\n");

	for (i = 0; i < 5; ++i)
		puts("");

	while (1) {
		puts(curr_sta->desc);
		if (!strcmp(curr_sta->actions[0], "$"))
			goto gameover;
donald:		printf("]=> ");
		fgets(buf, 128, stdin);
		buf[strlen(buf) - 1] = '\0';	/* eat the fucking \n */
		for (i = 0; i < curr_sta->action_count; ++i) {
			if (!strcasecmp(curr_sta->actions[i], buf)) {
				curr_sta = curr_sta->dest[i];
				goto gerald;
			}
		}
		if (*buf)
			printf("u wot ?\n");
		goto donald;
		gerald:	;
	}

gameover:
	printf("Game over\n");
}

