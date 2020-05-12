#define _GNU_SOURCE

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include "smap.h"
#include "entry.h"


typedef struct player
{
	char *name;
	double *score_and_wins;
} player;


void free_int_arrays(const char *key, void *value, void *arg)
{
	free(value);
}


// compares two Players by their score averages
int compare_by_score(const void *p1, const void *p2)
{
	double p1_s = ((struct player *)p1)->score_and_wins[0] / ((struct player *)p1)->score_and_wins[2];
	double p2_s = ((struct player *)p2)->score_and_wins[0] / ((struct player *)p2)->score_and_wins[2];

	char *p1_n = ((struct player *)p1)->name;
	char *p2_n = ((struct player *)p2)->name;

	if (p1_s < p2_s)
	{
		return 1;
	}
	else if (p1_s > p2_s)
	{
		return -1;
	}
	else
	{
		return strcmp(p1_n, p2_n);
	}
}

int compare_by_wins(const void *p1, const void *p2)
{
	double p1_w = ((struct player *)p1)->score_and_wins[1] / ((struct player *)p1)->score_and_wins[2];
	double p2_w = ((struct player *)p2)->score_and_wins[1] / ((struct player *)p2)->score_and_wins[2];

	char *p1_n = ((struct player *)p1)->name;
	char *p2_n = ((struct player *)p2)->name;

	if (p1_w < p2_w)
	{
		return 1;
	}
	else if (p1_w > p2_w)
	{
		return -1;
	}
	else
	{
		return strcmp(p1_n, p2_n);
	}
}

/**
* Calculates the winner based on points
*	@param map smap of the blotto players and their distributions
*	@param p1, p2 two keys: the names of the players
*	@param battle_worths the value of each battlefield
*	@param num_battlefields the number of battles
*	@return a double array of the players points in the order given
*/
double *calc_battle(smap *map, int num_battlefields, int* battle_worths, char* p1, char* p2)
{
	// leaving third position open for total faceoffs later
	double *points = calloc(3, sizeof(double));

	int* p1_moves = smap_get(map, p1);
	int* p2_moves = smap_get(map, p2);

	for (int i = 0; i < num_battlefields; i++)
	{
		if (p1_moves[i] > p2_moves[i])
		{
			points[0] += battle_worths[i];
		}
		else if (p1_moves[i] < p2_moves[i])
		{
			points[1] += battle_worths[i];
		}
		else // tie
		{
			points[0] += 0.5 * ((double) battle_worths[i]);
			points[1] += 0.5 * ((double) battle_worths[i]);
		}
	}

	return points;
}

int main(int argc, char **argv)
{

	// INVALID INPUT CHECKING
	if (argc < 4)
	{
		fprintf(stderr, "Not enough arguments\n");
		return 1;
	}

	FILE *matchups = fopen(argv[1], "r");
	if (matchups == NULL)
	{
		fprintf(stderr, "Could not open %s\n", argv[1]);
		return 1;
	}

	if (strcmp(argv[2], "score") != 0 && strcmp(argv[2], "win") != 0)
	{
		fprintf(stderr, "Not win or score\n");
		return 1;
	}

	// CONDUCTING BATTLES

	// instantiating and filling battle_worths with stdin, e.g. [1, 2, 3, 4]
	int num_battlefields = argc - 3;
	int battle_worths[num_battlefields];
	
	int count = 3;
	for (int i = 0; i < num_battlefields; i++)
	{
		battle_worths[i] = (int) atoi(argv[count]);
		count++;
	}

	// map:	names, distributions
	// score_map: names, arrays of [score, wins, total games]
	smap *map = smap_create(*smap_default_hash);
	smap *score_map = smap_create(*smap_default_hash);
	
	int num_coins = 0;
	bool num_coins_defined = false;

	char c;
	while ((c = getchar()) != EOF)
	{
		ungetc(c, stdin);
		char e32[32];
		entry_id e = entry_read(stdin, 33, num_battlefields);


		if (e.id != NULL && strcmp(e.id, "") != 0 && e.distribution != NULL && strlen(e.id) <= 32)
		{
			for (int i = 0; i < 32; i++)
			{
				e32[i] = e.id[i];
			}

			smap_put(map, e32, e.distribution);
			smap_put(score_map, e32, calloc(3, sizeof(double)));
		}
		else
		{
			fprintf(stderr, "Error (null entry)\n");
			return 1;
		}

		// verifying equal wallets
		if (!num_coins_defined)
		{
			for (int i = 0; i < num_battlefields; i++)
			{
				num_coins += e.distribution[i];
			}
			num_coins_defined = true;
		}
		
		int this_num_coins = 0;
		for (int i = 0; i < num_battlefields; i++)
		{
			this_num_coins += e.distribution[i];
		}
		
		if (this_num_coins != num_coins)
		{
			fprintf(stderr, "Inconsistent wallets\n");
			return 1;
		}
		//

		free(e.id);
	}

	// processing matchups
	char *buffer = NULL;
	size_t size = 0;
	int num_faceoffs = 0;

	while ((getline(&buffer, &size, matchups) != -1))
	{
		char p1_big[40];
		char p2_big[40];

		int s = sscanf(buffer, "%s %s", p1_big, p2_big);

		if (strlen(p1_big) > 32 || strlen(p2_big) > 32)
		{
			fprintf(stderr, "Long ID in entries\n");
			return 1;
		}

		// creating player IDs
		char p1[32];
		char p2[32];

		for (int i = 0; i < 32; i++)
		{
			p1[i] = p1_big[i];
			p2[i] = p2_big[i];
		}

		// increasing number of battles (faceoffs)
		if (s == 2)
		{
			num_faceoffs++;
		}
		else if (s == 1)
		{
			fprintf(stderr, "Missing entry ID in matchup");
		}

		// adjusting scores in score_map
		if (!smap_contains_key(map, p1) || !smap_contains_key(map, p2))
		{
			fprintf(stderr, "Invalid matchup ID\n");
			return 1;
		}

		double *score = calc_battle(map, num_battlefields, battle_worths, p1, p2);
		double *p1_score = smap_get(score_map, p1);
		double *p2_score = smap_get(score_map, p2);

		p1_score[0] += score[0];
		p2_score[0] += score[1];

		// adjusting number of wins in score_map
		if (score[0] > score[1])
		{
			p1_score[1]++;
		}
		else if (score[0] < score[1])
		{
			p2_score[1]++;
		}
		else
		{
			p1_score[1] += .5;
			p2_score[1] += .5;
		}

		// adjusting number of games played in score_map
		p1_score[2]++;
		p2_score[2]++;

		free(score);
	}


	fclose(matchups);
		

	/* Creating array of players
	 * Each of which has a name and a double array containing
	 * at position 0 their score, and at position 1 their win count 
	*/
	if (smap_size(score_map) != 0)
	{
		int score_size = smap_size(score_map);
		const char **score_keys = smap_keys(score_map);
		player *players = calloc(score_size, sizeof(player)); 
		
		// consolidates the spread-out data from the score_map hashmap into a traversable array
		for (int i = 0; i < score_size; i++)
		{
			players[i].name = strdup(score_keys[i]);
			players[i].score_and_wins = smap_get(score_map, score_keys[i]);
		}

		// results by score average
		if (strcmp(argv[2], "score") == 0)
		{
			qsort((void*)players, score_size, sizeof(players[0]), compare_by_score);

			// justified formatting
			for (int i = 0; i < score_size; i++)
			{
				double s = players[i].score_and_wins[0] / players[i].score_and_wins[2];

				if (s < 10)
				{
					printf("  ");
				}
				else if (s < 100)
				{
					printf(" ");
				}

				printf("%.3lf %s\n", s, players[i].name);
			}
		}
		// results by win average
		else if (strcmp(argv[2], "win") == 0)
		{
			qsort((void*)players, score_size, sizeof(players[0]), compare_by_wins);

			for (int i = 0; i < score_size; i++)
			{
				double s = players[i].score_and_wins[1] / players[i].score_and_wins[2];

				if (s < 10)
				{
					printf("  ");
				}
				else if (s < 100)
				{
					printf(" ");
				}
				
				printf("%.3lf %s\n", s, players[i].name);
			}
		}

		for (int i = 0; i < score_size; i++)
		{
			free(players[i].name);
		}

		free(players);
		free(buffer);
		free(score_keys);

		smap_for_each(score_map, free_int_arrays, NULL);
		smap_destroy(score_map);

		smap_for_each(map, free_int_arrays, NULL);
		smap_destroy(map);
	}
	else
	{
		return 1;
	}
}
