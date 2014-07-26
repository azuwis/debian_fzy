#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#include "fzy.h"

#define SCORE_MIN -1

int has_match(const char *needle, const char *haystack){
	while(*needle){
		if(!*haystack)
			return 0;
		while(*haystack && tolower(*needle) == tolower(*haystack++))
			needle++;
	}
	return 1;
}

/* print one of the internal matrices */
void mat_print(int *mat, int n, int m){
	int i, j;
	for(i = 0; i < n; i++){
		for(j = 0; j < m; j++){
			fprintf(stderr, " %3i", mat[i*m + j]);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n\n");
}

#define max(a, b) (((a) > (b)) ? (a) : (b))
typedef int score_t;

double calculate_score(const char *needle, const char *haystack, size_t *positions){
	if(!*haystack || !*needle)
		return SCORE_MIN;

	int n = strlen(needle);
	int m = strlen(haystack);

	if(m > 1024){
		/*
		 * Unreasonably large candidate: return no score
		 * If it is a valid match it will still be returned, it will
		 * just be ranked below any reasonably sized candidates
		 */
		return 0;
	}

	int bow[m];
	int D[n][m], M[n][m];
	bzero(D, sizeof(D));
	bzero(M, sizeof(M));

	/*
	 * D[][] Stores the best score for this position ending with a match.
	 * M[][] Stores the best possible score at this position.
	 */

	/* Which positions are beginning of words */
	int at_bow = 1;
	char last_ch = '\0';
	for(int i = 0; i < m; i++){
		char ch = haystack[i];
		/* TODO: What about allcaps (ex. README) */
		bow[i] = (at_bow && isalnum(ch)) || (isupper(ch) && !isupper(last_ch));
		at_bow = !isalnum(ch);
		last_ch = ch;
	}

	for(int i = 0; i < n; i++){
		for(int j = 0; j < m; j++){
			int match = tolower(needle[i]) == tolower(haystack[j]);
			if(match){
				score_t score = 0;
				if(i && j)
					score = M[i-1][j-1];
				if(bow[j])
					score += 2;
				else if(i && j && D[i-1][j-1])
					score = max(score, 1 + D[i-1][j-1]);
				M[i][j] = D[i][j] = score;
			}
			if(j)
				M[i][j] = max(M[i][j], M[i][j-1]);
		}
	}

	/* backtrace to find the positions of optimal matching */
	if(positions){
		for(int i = n-1, j = m-1; i >= 0; i--){
			int last = M[i][j];
			for(; j >= 0 && M[i][j] == last; j--){
				/*
				 * There may be multiple paths which result in
				 * the optimal weight.
				 *
				 * Since we don't exit the loop on the first
				 * match, positions[i] may be assigned to
				 * multiple times. Since we are decrementing i
				 * and j, this favours the optimal path
				 * occurring earlier in the string.
				 */
				if(tolower(needle[i]) == tolower(haystack[j])){
					positions[i] = j;
				}
			}
		}
	}

	return (float)(M[n-1][m-1]) / (float)(n * 2 + 1);
}

double match_positions(const char *needle, const char *haystack, size_t *positions){
	if(!*needle){
		return 1.0;
	}else if(!has_match(needle, haystack)){
		return SCORE_MIN;
	}else if(!strcasecmp(needle, haystack)){
		if(positions){
			int n = strlen(needle);
			for(int i = 0; i < n; i++)
				positions[i] = i;
		}
		return 1.0;
	}else{
		return calculate_score(needle, haystack, positions);
	}
}

double match(const char *needle, const char *haystack){
	return match_positions(needle, haystack, NULL);
}
