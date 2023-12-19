#pragma once

#include <base.h>

TSTRUCT(intLinkedHashListBucket){
	intLinkedHashListBucket *prev, *next;
	char *key;
	int keylen;
	int value;
};

TSTRUCT(intLinkedHashList){
	int total, used, tombstones;
	intLinkedHashListBucket *buckets, *first, *last;
};

#define TOMBSTONE UINTPTR_MAX

intLinkedHashListBucket *intLinkedHashListGet(intLinkedHashList *list, char *key, int keylen);

intLinkedHashListBucket *intLinkedHashListGetChecked(intLinkedHashList *list, char *key, int keylen);

void intLinkedHashListRemove(intLinkedHashList *list, intLinkedHashListBucket *b);

intLinkedHashListBucket *intLinkedHashListNew(intLinkedHashList *list, char *key, int keylen);

enum WordType {
	NOUN = 1,
	VERB = NOUN<<1,
	ADVERB = VERB<<1,
	ADJECTIVE = ADVERB<<1,
	CONJUNCTION = ADJECTIVE<<1,
	ABBREVIATION = CONJUNCTION<<1,
	PREPOSITION = ABBREVIATION<<1,
	PRONOUN = PREPOSITION<<1,
	INTERJECTION = PRONOUN<<1,
};

TSTRUCT(Lexer){
	char *prev, *cur, *end;
};

void parse_dictionary_file();

int get_word_type(char *str, int len);

char *get_word_type_string(int type);

void print_word_type(char *cstr);