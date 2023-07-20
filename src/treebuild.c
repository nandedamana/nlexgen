#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "treebuild.h"

void vstring_construct(vstring *this, const char * inits)
{
	size_t newlen = strlen(inits);
	size_t newsiz = newlen + 1;
	this->s = NULL;
	this->s = realloc(this->s, newsiz * sizeof(this->s[0]));
	if(this->s == NULL) { perror(NULL); exit(EXIT_FAILURE); }

	memcpy(this->s, inits, newsiz);
	this->len = newlen;
}

void vstring_append(vstring *this, const char * suffix)
{
	size_t suflen = strlen(suffix);
	size_t newlen = this->len + suflen;
	size_t newsiz = newlen + 1;
	this->s = realloc(this->s, newsiz * sizeof(this->s[0]));
	if(this->s == NULL) { perror(NULL); exit(EXIT_FAILURE); }

	memcpy(this->s + this->len, suffix, suflen + 1);
	this->len = newlen;
}

void vstring_appendc(vstring *this, char suffix)
{
	size_t newsiz = this->len + 2;
	this->s = realloc(this->s, newsiz * sizeof(this->s[0]));
	if(this->s == NULL) { perror(NULL); exit(EXIT_FAILURE); }

	this->s[this->len] = suffix;
	this->s[this->len + 1] = '\0';
	this->len += 1;
}

void vstring_clear(vstring *this)
{
	this->len = 0;
	free(this->s);
	this->s = NULL;
	this->s = realloc(this->s, 1 * sizeof(this->s[0]));
	if(this->s == NULL) { perror(NULL); exit(EXIT_FAILURE); }

	this->s[0] = '\0';
}

char * vstring_detach(vstring *this)
{
	char *retval = this->s;
	this->len = 0;
	this->s = NULL;

	return retval;
}

void vstring_destruct(vstring *this)
{
	if(this->s) {
		free(this->s);
	}

	this->s = NULL;
}

_ngg_tuple_nlg_get_rule nlg_get_rule(NlexHandle *nh)
{

	vstring * vspat = malloc(sizeof(vstring));
	if(vspat == NULL) { perror(NULL); exit(EXIT_FAILURE); }
	vstring_construct(vspat, "");

	vstring * vsact = malloc(sizeof(vstring));
	if(vsact == NULL) { perror(NULL); exit(EXIT_FAILURE); }
	vstring_construct(vsact, "");
	_Bool in_list = false;
	_Bool escaped = false;
	while(true) {
		int ch = nlex_next(nh);
		if(ch == '\r' || ch == '\n' || ch == EOF || ch == '\0') {
			if((vstring_get_length(vspat)) > 0) {
				vstring_destruct(vspat);
				free(vspat);
				vspat = NULL;
				vstring_destruct(vsact);
				free(vsact);
				vsact = NULL;
				return (_ngg_tuple_nlg_get_rule){NULL, NULL, NLEXERR_NO_ACT_GIVEN};
			} else {
				vstring_destruct(vspat);
				free(vspat);
				vspat = NULL;
				vstring_destruct(vsact);
				free(vsact);
				vsact = NULL;
				return (_ngg_tuple_nlg_get_rule){NULL, NULL, NLEXERR_SUCCESS};
			}

		}

		if(ch == '[') {
			if(!escaped) {
				in_list = true;
			}

		} else if(ch == ']') {
			if(!escaped) {
				in_list = false;
			}

		}

		if(escaped) {
			escaped = false;
		} else if(ch == '\\') {
			escaped = true;
		}

		if(ch == '\t') {
			break;
		} else if((ch == ' ') && (!in_list)) {
			while((nlex_next(nh)) == ' ') {
			}

			break;
		}

		vstring_appendc(vspat, ch);
	}

	vstring_appendc(vsact, nlex_last(nh));
	while(true) {
		int ch = nlex_next(nh);
		if(ch == '\r' || ch == '\n' || ch == EOF || ch == '\0') {
			break;
		}

		vstring_appendc(vsact, ch);
	}

	char * pat = vstring_detach(vspat);
	char * act = vstring_detach(vsact);

	vstring_destruct(vspat);
	free(vspat);
	vspat = NULL;
	vstring_destruct(vsact);
	free(vsact);
	vsact = NULL;
	return (_ngg_tuple_nlg_get_rule){pat, act, NLEXERR_SUCCESS};
}

const char * nlg_build_tree(NanTreeNode *root, NlexHandle *nh)
{
	nlg_tree_init_root(root);
	while(true) {
		_ngg_tuple_nlg_get_rule _ngg_tmp_0 = nlg_get_rule(nh);
		const char * err = _ngg_tmp_0.m2;
		char * act = _ngg_tmp_0.m1;
		char * pat = _ngg_tmp_0.m0;
		char * pat_autodel = pat;
		if(NLEXERR_SUCCESS != err) {
			free(pat_autodel);
			pat_autodel = NULL;
			return err;
		} else if(!pat) {
			free(pat_autodel);
			pat_autodel = NULL;
			return NLEXERR_SUCCESS;
		}

		assert(pat); /* treebuild.ngg:103 */
		assert(act); /* treebuild.ngg:103 */
		err = nlg_tree_add_rule(root, nh, pat, act);
		if(NLEXERR_SUCCESS != err) {
			free(pat_autodel);
			pat_autodel = NULL;
			return err;
		}

		free(pat_autodel);
		pat_autodel = NULL;
	}


	return NLEXERR_SUCCESS;
}

_ngg_tuple_nlg_get_rule _ngg_tuple_nlg_get_rule_default()
{
	_ngg_tuple_nlg_get_rule s;
	s.m0 = NULL;
	s.m1 = NULL;
	s.m2 = NULL;

	return s;
}

