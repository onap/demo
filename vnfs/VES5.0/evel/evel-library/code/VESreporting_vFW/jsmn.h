/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 *
 ****************************************************************************/

#ifndef __JSMN_H_
#define __JSMN_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_PRIMITIVE = 4
} jsmntype_t;

enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
};

typedef struct arrayValues {
   char arrayString[32];
} ARRAYVAL;

typedef struct keyValResult {
   char keyStr[80];
   char valStr[250];
   char resultStr[80];
} KEYVALRESULT;

/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct {
	jsmntype_t type;
	int start;
	int end;
	int size;
//#ifdef JSMN_PARENT_LINKS
	int parent;
//#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;

#define TOKEN_EQ(t, tok_start, tok_end, tok_type) \
        ((t).start == tok_start \
         && (t).end == tok_end  \
         && (t).type == (tok_type))

#define TOKEN_STRING(js, t, s) \
        (strncmp(js+(t).start, s, (t).end - (t).start) == 0 \
         && strlen(s) == (t).end - (t).start)

#define TOKEN_COPY(js, t, s) \
        strncpy(s, js+(t).start, (t).end - (t).start)

#define TOKEN_PRINT(t) \
        printf("start: %d, end: %d, type: %d, size: %d\n", \
          (t).start, (t).end, (t).type, (t).size)


/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens);

int jsoneq(const char *json, jsmntok_t *tok, const char *s);

void printToken(char * js, jsmntok_t * tokens, int numToken);

int getStringToken(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, char * value, int maxValueSize);

int getIntToken(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, int * value);

int getArrayTokens(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, ARRAYVAL * arrayValue, int * numElements);

int isTokenPresent(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam);

int read_keyVal_params(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, KEYVALRESULT * keyValueResultList, int * numElements);

int getStringTokenV2(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, char *gParam, char * value, int maxValueSize);

#ifdef __cplusplus
}
#endif

#endif /* __JSMN_H_ */
