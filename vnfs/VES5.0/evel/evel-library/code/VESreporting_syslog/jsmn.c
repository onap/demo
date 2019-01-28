/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Unless otherwise specified, all software contained herein is
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
 * ECOMP is a trademark and service mark of AT&T Intellectual Property.
 ****************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "jsmn.h"

/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser,
		jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *tok;
	if (parser->toknext >= num_tokens) {
		return NULL;
	}
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
//#ifdef JSMN_PARENT_LINKS
	tok->parent = -1;
//#endif
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type,
                            int start, int end) {
//printf("jsmn_fill_token:: start-%d, end-%d\n", start, end);
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
		size_t len, jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *token;
	int start;

	start = parser->pos;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
//printf("jsmn_parse_primitive:: the char is - %c\n", js[parser->pos]);
		switch (js[parser->pos]) {
#ifndef JSMN_STRICT
			/* In strict mode primitive must be followed by "," or "}" or "]" */
			case ':':
#endif
			case '\t' : case '\r' : case '\n' : case ' ' :
			case ','  : case ']'  : case '}' :
				goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}
#ifdef JSMN_STRICT
	/* In strict mode primitive must be followed by a comma/object/array */
	parser->pos = start;
	return JSMN_ERROR_PART;
#endif

found:
	if (tokens == NULL) {
		parser->pos--;
		return 0;
	}
	token = jsmn_alloc_token(parser, tokens, num_tokens);
	if (token == NULL) {
		parser->pos = start;
		return JSMN_ERROR_NOMEM;
	}
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
//#ifdef JSMN_PARENT_LINKS
	token->parent = parser->toksuper;
//#endif
	parser->pos--;
	return 0;
}

/**
 * Fills next token with JSON string.
 */
static int jsmn_parse_string(jsmn_parser *parser, const char *js,
		size_t len, jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *token;

	int start = parser->pos;

	parser->pos++;

	/* Skip starting quote */
	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c = js[parser->pos];
		char d = js[parser->pos + 1];
//		char e = js[parser->pos + 2];
//printf("jsmn_parse_string: value-%c, pos-%d\n", c,parser->pos);

		/* Quote: end of string */
//		if (c == '\"') {
		if (d == '\"') {
//		if ((d == '\"')&&((e == ' ')||(e == ','))) {
parser->pos++;
//printf("jsmn_parse_string: end of string\n");
			if (tokens == NULL) {
//printf("jsmn_parse_string: end tokens is NULL\n");
				return 0;
			}
			token = jsmn_alloc_token(parser, tokens, num_tokens);
//printf("jsmn_parse_string: Allocated tokens \n");
			if (token == NULL) {
//printf("jsmn_parse_string: Allocated tokens is NULL\n");
				parser->pos = start;
				return JSMN_ERROR_NOMEM;
			}
			jsmn_fill_token(token, JSMN_STRING, start+1, parser->pos);
//printf("jsmn_parse_string: Allocated tokens is filled\n");
//#ifdef JSMN_PARENT_LINKS
			token->parent = parser->toksuper;
//#endif
			return 0;
		}

		/* Backslash: Quoted symbol expected */
		if (c == '\\' && parser->pos + 1 < len) {
			int i;
			parser->pos++;
//printf("jsmn_parse_string: value - %c, POS-%3d \n",c, js[parser->pos]);
			switch (js[parser->pos]) {
				/* Allowed escaped symbols */
				case '\"': case '/' : case '\\' : case 'b' :
				case 'f' : case 'r' : case 'n'  : case 't' :
					break;
				/* Allows escaped symbol \uXXXX */
				case 'u':
					parser->pos++;
					for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
						/* If it isn't a hex character we have an error */
						if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
									(js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
									(js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
							parser->pos = start;
							return JSMN_ERROR_INVAL;
						}
						parser->pos++;
					}
					parser->pos--;
					break;
				/* Unexpected symbol */
				default:
					parser->pos = start;
					return JSMN_ERROR_INVAL;
			}
		}
	}
	parser->pos = start;
printf("jsmn_parse_string: exiting with ERROR_PART, pos-%d", parser->pos);
	return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens) {
	int r;
	int i;
	jsmntok_t *token;
	int count = parser->toknext;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c;
		jsmntype_t type;

		c = js[parser->pos];
//printf("jsmn_parse: value of c - %c\n",c);
		switch (c) {
			case '{': case '[':
				count++;
				if (tokens == NULL) {
					break;
				}
				token = jsmn_alloc_token(parser, tokens, num_tokens);
				if (token == NULL)
					return JSMN_ERROR_NOMEM;
				if (parser->toksuper != -1) {
					tokens[parser->toksuper].size++;
//#ifdef JSMN_PARENT_LINKS
					token->parent = parser->toksuper;
//#endif
				}
				token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
				token->start = parser->pos;
				parser->toksuper = parser->toknext - 1;
				break;
			case '}': case ']':
				if (tokens == NULL)
					break;
				type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
//#ifdef JSMN_PARENT_LINKS
				if (parser->toknext < 1) {
					return JSMN_ERROR_INVAL;
				}
				token = &tokens[parser->toknext - 1];
				for (;;) {
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						token->end = parser->pos + 1;
						parser->toksuper = token->parent;
						break;
					}
					if (token->parent == -1) {
						break;
					}
					token = &tokens[token->parent];
				}
//#else
//				for (i = parser->toknext - 1; i >= 0; i--) {
//					token = &tokens[i];
//					if (token->start != -1 && token->end == -1) {
//						if (token->type != type) {
//							return JSMN_ERROR_INVAL;
//						}
//						parser->toksuper = -1;
//						token->end = parser->pos + 1;
//						break;
//					}
//				}
//				/* Error if unmatched closing bracket */
//				if (i == -1) return JSMN_ERROR_INVAL;
//				for (; i >= 0; i--) {
//					token = &tokens[i];
//					if (token->start != -1 && token->end == -1) {
//						parser->toksuper = i;
//						break;
//					}
//				}
//#endif
				break;
			case '\"':
				r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;
			case '\t' : case '\r' : case '\n' : case ' ':
				break;
			case ':':
				parser->toksuper = parser->toknext - 1;
//printf("jsmn_parse: value of c is :: - %c\n",c);
				break;
			case ',':
				if (tokens != NULL && parser->toksuper != -1 &&
						tokens[parser->toksuper].type != JSMN_ARRAY &&
						tokens[parser->toksuper].type != JSMN_OBJECT) {
//#ifdef JSMN_PARENT_LINKS
					parser->toksuper = tokens[parser->toksuper].parent;
//#else
//					for (i = parser->toknext - 1; i >= 0; i--) {
//						if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
//							if (tokens[i].start != -1 && tokens[i].end == -1) {
//								parser->toksuper = i;
//								break;
//							}
//						}
//					}
//#endif
				}
				break;
#ifdef JSMN_STRICT
			/* In strict mode primitives are: numbers and booleans */
			case '-': case '0': case '1' : case '2': case '3' : case '4':
			case '5': case '6': case '7' : case '8': case '9':
			case 't': case 'f': case 'n' :
				/* And they must not be keys of the object */
				if (tokens != NULL && parser->toksuper != -1) {
					jsmntok_t *t = &tokens[parser->toksuper];
					if (t->type == JSMN_OBJECT ||
							(t->type == JSMN_STRING && t->size != 0)) {
						return JSMN_ERROR_INVAL;
					}
				}
#else
			/* In non-strict mode every unquoted value is a primitive */
			default:
#endif
				r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 && tokens != NULL)
					tokens[parser->toksuper].size++;
				break;

#ifdef JSMN_STRICT
			/* Unexpected char in strict mode */
			default:
				return JSMN_ERROR_INVAL;
#endif
		}
	}

	if (tokens != NULL) {
		for (i = parser->toknext - 1; i >= 0; i--) {
//printf("index -%2d, start is %3d, end is %3d\n", i, tokens[i].start, tokens[i].end);
			/* Unmatched opened object or array */
			if (tokens[i].start != -1 && tokens[i].end == -1) {
				return JSMN_ERROR_PART;
			}
		}
	}

	return count;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void jsmn_init(jsmn_parser *parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}

int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
   if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
       strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
          return 0;
   }
   return -1;
}

void printToken(char * js, jsmntok_t * tokens, int numToken)
{
  for (int i = 1; i < numToken; i++)
  {
     printf("Token number-%2d, parent-%2d, type-%d size-%2d, parameter -", i, tokens[i].parent, tokens[i].type, tokens[i].size);
     if (tokens[i].type == JSMN_STRING || tokens[i].type == JSMN_PRIMITIVE) {
        printf("%.*s\n", tokens[i].end - tokens[i].start, js + tokens[i].start);
     } else if (tokens[i].type == JSMN_ARRAY) {
        printf("[%d elems]\n", tokens[i].size);
     } else if (tokens[i].type == JSMN_OBJECT) {
        printf("{%d elems}\n", tokens[i].size);
     } else {
        printf("value?? - ");
        TOKEN_PRINT(tokens[i]);
     }
  }
}

int getStringToken(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, char * value, int maxValueSize)
{
  int i = 0;
  int dpToken = 0;

  memset(value, 0, maxValueSize);

  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], pParam) == 0)
        break;
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 1; //Parent token not seen
  }
  for (i=1; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
        if(jsoneq(js, &tokens[i], param) != 0)
           continue;
        TOKEN_COPY(js, tokens[i+1], value);

        return 0; //Success
     }
  }
  printf("The parameter %s is not present in JSON file\n", param);
  return 2; //parameter not found
}


int getStringTokenV2(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, char * gParam,char * value, int maxValueSize)
{
  int i = 0;
  int dpToken = 0;

  memset(value, 0, maxValueSize);

  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], gParam) == 0)
        break;
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 1; //Grand Parent token not seen
  }

  for (i=dpToken; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
        if(jsoneq(js, &tokens[i], pParam) == 0)
           break;
     }
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 2; //Parent token not seen
  }

  for (i=dpToken; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
        if(jsoneq(js, &tokens[i], param) != 0)
           continue;
        TOKEN_COPY(js, tokens[i+1], value);

        return 0; //Success
     }
  }

  printf("The parameter %s is not present in JSON file\n", param);
  return 2; //parameter not found
}

int getIntToken(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, int * value)
{
  int i = 0;
  int dpToken = 0;
  char  val[128];

  memset(val, 0, 128);
  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], pParam) == 0)
        break;
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 1; //Parent token not seen
  }
  for (i=1; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
        if(jsoneq(js, &tokens[i], param) != 0)
        {
           continue;
        }

        if(tokens[i+1].type != JSMN_PRIMITIVE)
          return 3; //Wrong parameter type

//        printf("INT parameter / Value - %s", param);
        TOKEN_COPY(js, tokens[i+1], val);
        *value = atoi(val);
//        printf(" -  %d\n", *value);

        return 0; //success
     }
  }
  printf("The parameter %s is not present in JSON file\n", param);
  return 2; //parameter not found
}

void parseDirectParameters(char * js, jsmntok_t * tokens, int numToken)
{
  int i = 0;
  int dpToken = 0;
  char  param[128];
  char  value[128];

  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], "directParameters") == 0)
        break;
  }

  if (i < numToken)
  {
    dpToken = ++i;
  }

  for (int i = 1; i < numToken; i++)
  {
     memset(param, 0, 128);
     memset(value, 0, 128);
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
        TOKEN_COPY(js, tokens[i], param);
//        printf("parameter / Value - %s", param);
        TOKEN_COPY(js, tokens[i+1], value);
//        printf(" - %s\n", value);
     }
  }
}

int getArrayTokens(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, ARRAYVAL * arrayValue, int * numElements)
{
  int i = 0;
  int dpToken = 0;

  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], pParam) == 0)
        break;
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {    
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 1; //Parent token not seen
  }
  for (i=1; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
//        printf("value of token %d\n", i);
        if(jsoneq(js, &tokens[i], param) != 0)
           continue;

        if (tokens[i+1].type == JSMN_ARRAY)
        {
           *numElements = tokens[i+1].size;
//           printf("[%d elems]\n", *numElements);

           for (int k = 0; k < *numElements; k++)
           {
              TOKEN_COPY(js, tokens[i+2+k], arrayValue[k].arrayString);
//              printf(" - %s\n", arrayValue[k].arrayString);
           }
           return 0; //Success
        }
     }
  }
  printf("The parameter %s is not present in JSON file\n", param);
  return 2; //parameter not found
}

int isTokenPresent(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam)
{
  int i = 0;
  int dpToken = 0;
  char  val[128];

  memset(val, 0, 128);
  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], pParam) == 0)
        break;
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 1; //Parent token not seen
  }
  for (i=1; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING)
     {
        if(jsoneq(js, &tokens[i], param) == 0)
        {
           return 0; //Token present
        }
     }
  }
  printf("The parameter %s is not present in JSON file\n", param);
  return 2; //Token Not present
}

int read_keyVal_params(char * js, jsmntok_t * tokens, int numToken, char * param, char * pParam, KEYVALRESULT * keyValueResultList, int * numElements)
{
  int i = 0;
  int dpToken = 0;

  for (i = 1; i < numToken; i++)
  {
     if(jsoneq(js, &tokens[i], pParam) == 0)
        break;
  }
  if (i < numToken)
  {
    dpToken = ++i;
  }
  else
  {
    printf("The parameter %s is not present in JSON file\n", pParam);
    return 1; //Parent token not seen
  }
  for (i=1; i < numToken; i++)
  {
     if (tokens[i].parent == dpToken && tokens[i].type == JSMN_STRING && tokens[i].size == 1)
     {
//        printf("value of token %d\n", i);
        if(jsoneq(js, &tokens[i], param) != 0)
           continue;

        if (tokens[i+1].type == JSMN_OBJECT)
        {
           *numElements = tokens[i+1].size;
//           printf("{%d elems}\n", *numElements);

           for (int k = 0; k < *numElements; k++)
           {
              TOKEN_COPY(js, tokens[i+2+k*2], keyValueResultList[k].keyStr);
//              printf("Key - %s", keyValueResultList[k].keyStr);
              TOKEN_COPY(js, tokens[i+3+k*2], keyValueResultList[k].valStr);
//              printf("Value - %s\n", keyValueResultList[k].valStr);
           }
           return 0; //Success
        }
     }
  }
  printf("The parameter %s is not present in JSON file\n", param);
  return 2; //parameter not found
}
