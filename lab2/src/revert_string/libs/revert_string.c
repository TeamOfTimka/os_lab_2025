#include "revert_string.h"
#include <stdlib.h>
#include <string.h>

void RevertString(char *str)
{
	int len_str = (strlen(str));
	char *reverted_str = malloc(sizeof(char) * len_str);

	for (int i = 0; i < len_str; i++)
	{
		reverted_str[i] = str[len_str - i - 1];
	}

	strcpy(str, reverted_str);
	free(reverted_str);
}

