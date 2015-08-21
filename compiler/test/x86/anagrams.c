/* -rw-r--r-- 1 blockeduser blockeduser 1710 Sep  1  2014 anagrams.c */

/*
 * http://www.geeksforgeeks.org/microsoft-interview-set-20-campus-internship/
 * 
 * "Given a word and a text, return the count of the occurences of anagrams of the word in the text.
 * For eg. word is “for” and the text is “forxxorfxdofr”, anagrams of “for” will be “ofr”, “orf”,”fro”,
 * etc. So the answer would be 3 for this particular example."
 */

#include <stdlib.h>

main() {
	printf("%d\n",
		count_anagrams("for", "forxxorfxdofr"));
	printf("%d\n",
		count_anagrams("abc", "aaaaababababaaabbbbccccbcbabcbcabcabc"));
	printf("%d\n",
	        count_anagrams("oboe", "obooooooe...booe...ooeb...oboe...oeob"));
	printf("%d\n",
	       count_anagrams("ooo", "oooooooooooooooooooooooooooooo  ooaooaoao oo"));
}

typedef struct trie_tag {
  struct trie_tag *map[256];
  int val;
} trie_t;

trie_t *new_trie() {
   trie_t *ptr = malloc(sizeof(trie_t));
   ptr->val = 0;
   int i;
   for (i = 0; i < 256; ++i)
     ptr->map[i] = NULL;
   return ptr;
}

void make_anagrams(trie_t *tp, char *word) {
 char word_copy[256];
 int i;
 int avail = 0;
 for (i = 0; word[i]; ++i) { 
  if (word[i] != '!') {
    ++avail;
    strcpy(word_copy, word);
    word_copy[i] = '!';
    tp->map[word[i]] = new_trie();
    make_anagrams(tp->map[word[i]], word_copy);
  }
 }
 
 if (avail == 0)
   tp->val = 1; 
}

int count_anagrams(char *word, char *text) {
  trie_t *root = new_trie();
  trie_t *ptr;
  int i, j, k;
  int count = 0;
  
  make_anagrams(root, word);
  ptr = root;
  for (i = 0; text[i]; ++i) { 
    if (ptr->map[text[i]] != NULL) {
      ptr = ptr->map[text[i]];
    } else {
      ptr = root;
    }
    if (ptr->val == 1) {
     ptr = root;
     ++count;
    }
  }
  return count;
}

