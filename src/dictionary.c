#include <dictionary.h>

intLinkedHashListBucket *intLinkedHashListGet(intLinkedHashList *list, char *key, int keylen){
	if (!list->total) return 0;
	int index = fnv_1a(key,keylen) % list->total;
	intLinkedHashListBucket *tombstone = 0;
	while (1){
		intLinkedHashListBucket *b = list->buckets+index;
		if (b->key == TOMBSTONE) tombstone = b;
		else if (b->key == 0) return tombstone ? tombstone : b;
		else if (!memcmp(b->key,key,keylen)) return b;
		index = (index + 1) % list->total;
	}
}

intLinkedHashListBucket *intLinkedHashListGetChecked(intLinkedHashList *list, char *key, int keylen){
	intLinkedHashListBucket *b = intLinkedHashListGet(list,key,keylen);
	if (!b || b->key == 0 || b->key == TOMBSTONE) return 0;
	return b;
}

void intLinkedHashListRemove(intLinkedHashList *list, intLinkedHashListBucket *b){
	b->key = TOMBSTONE;
	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;
	if (list->first == b) list->first = b->next;
	if (list->last == b) list->last = b->prev;
	list->used--;
	list->tombstones++;
}

intLinkedHashListBucket *intLinkedHashListNew(intLinkedHashList *list, char *key, int keylen){
	if ((list->used+list->tombstones+1) > (list->total*3)/4){ // 3/4 load limit
		intLinkedHashList newList;
		newList.total = 16; // same as used resize factor
		while (newList.total < (list->used*16)) newList.total *= 2; // 16x used resize
		newList.used = list->used;
		newList.tombstones = 0;
		newList.first = 0;
		newList.last = 0;
		newList.buckets = zalloc_or_die(newList.total*sizeof(*newList.buckets));
		for (intLinkedHashListBucket *b = list->first; b; b = b->next){
			intLinkedHashListBucket *nb = intLinkedHashListGet(&newList,b->key,b->keylen);
			nb->key = b->key;
			nb->keylen = b->keylen;
			nb->value = b->value;
			if (!newList.first){
				newList.first = nb;
				nb->prev = 0;
			} else {
				newList.last->next = nb;
				nb->prev = newList.last;
			}
			newList.last = nb;
			nb->next = 0;
		}
		if (list->buckets) free(list->buckets);
		*list = newList;
	}
	intLinkedHashListBucket *b = intLinkedHashListGet(list,key,keylen);
	b->key = key;
	b->keylen = keylen;
	if (!list->first){
		list->first = b;
		b->prev = 0;
	} else {
		list->last->next = b;
		b->prev = list->last;
	}
	list->last = b;
	b->next = 0;
	if (b->key == TOMBSTONE) list->tombstones--;
	list->used++;
	return b;
}

static void get_word(Lexer *l, String *w){
	while (1){
		if (l->cur == l->end || *l->cur == '\r' || *l->cur == '\n'){
			w->len = 0;
			w->data = 0;
			return;
		}
		if (is_alpha_numeric(*l->cur)) break;
		l->cur++;
	}
	l->prev = l->cur;
	while (l->cur != l->end && is_alpha_numeric(*l->cur)) l->cur++;
	w->len = l->cur-l->prev;
	w->data = l->prev;
}

static void advance_line(Lexer *l){
	while (l->cur != l->end && !(*l->cur == '\r' || *l->cur == '\n')) l->cur++;
	while (l->cur != l->end && !is_alpha_numeric(*l->cur)) l->cur++;
}

static String dictString;

static intLinkedHashList dict;

void parse_dictionary_file(){
	/*
	Parses OxfordEnglishDictionary.txt into a hash table of word->word_type.
	OxfordEnglishDictionary.txt uses the following codes for word types:

		NOUN, n
		VERB, v
		ADVERB, adv
		ADJECTIVE, adj
		CONJUNCTION, conj
		ABBREVIATION, abbr
		PREPOSITION, prep
		PRONOUN, pron
		INTERJECTION, int
	*/
	dictString.data = load_file("../res/OxfordEnglishDictionary.txt",&dictString.len);
	Lexer lexer = {
		.prev = dictString.data,
		.cur = dictString.data,
		.end = dictString.data+dictString.len
	};
	while (lexer.cur != lexer.end){
		String word,type;
		get_word(&lexer,&word);
		get_word(&lexer,&type);
		if (word.len && type.len){
			string_to_lower(word.len,word.data);//lower case the words so we can use switches
			string_to_lower(type.len,type.data);
			int typeVal = 0;
			switch (type.len){
				case 1:
					switch(type.data[0]){
						case 'n':
							typeVal = NOUN;
							break;
						case 'v':
							typeVal = VERB;
							break;
					}
					break;
				case 3:
					switch(type.data[0]){
						case 'a':
							if (type.data[1] == 'd'){
								switch(type.data[2]){
								case 'j':
									typeVal = ADJECTIVE;
									break;
								case 'v':
									typeVal = ADVERB;
									break;
								}
							}
							break;
						case 'i':
							if (memcmp(type.data+1,"nt",2)){
								typeVal = INTERJECTION;
							}
							break;
					}
				case 4:
					switch(type.data[0]){
						case 'a':
							if (!memcmp(type.data+1,"bbr",3)){
								typeVal = ABBREVIATION;
							}
							break;
						case 'c':
							if (!memcmp(type.data+1,"onj",3)){
								typeVal = CONJUNCTION;
							}
							break;
						case 'p':
							if (type.data[1] == 'r'){
								if (!memcmp(type.data+2,"ep",2)){
									typeVal = PREPOSITION;
								} else if (!memcmp(type.data+2,"on",2)){
									typeVal = PRONOUN;
								}
							}
							break;
					}
			}
			if (typeVal){
				intLinkedHashListBucket *b = intLinkedHashListGetChecked(&dict,word.data,word.len);
				if (!b){
					b = intLinkedHashListNew(&dict,word.data,word.len);
					b->value = typeVal;
				}
			}
		}
		advance_line(&lexer);
	}
}

int get_word_type(char *str, int len){
	intLinkedHashListBucket *b = intLinkedHashListGet(&dict,str,len);
	if (!b){
		return 0;
	}
	return b->value;
}

char *get_word_type_string(int type){
	char *s = "unknown";
	switch (type){
		case NOUN: s = "noun"; break;
		case VERB: s = "verb"; break;
		case ADVERB: s = "adverb"; break;
		case ADJECTIVE: s = "adjective"; break;
		case CONJUNCTION: s = "conjunction"; break;
		case ABBREVIATION: s = "abbreviation"; break;
		case PREPOSITION: s = "preposition"; break;
		case PRONOUN: s = "pronoun"; break;
		case INTERJECTION: s = "interjection"; break;
	}
	return s;
}

void print_word_type(char *cstr){
	printf("%s\n",get_word_type_string(get_word_type(cstr,strlen(cstr))));
}