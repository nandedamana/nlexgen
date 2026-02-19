#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#include "treebuild.h"

void vstring_construct(vstring *this, const char * inits)
{
	char *_tmp_2 = (char *) calloc((size_t) (strlen(inits) + 1u), sizeof(char));
	if(_tmp_2 == NULL) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	char * _tmp_1 = (char *) _tmp_2;
	memcpy(_tmp_1, inits, strlen(inits) + 1u);
	this->s = _tmp_1;
	this->len = strlen((char *) this->s);
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
	this->len = (unsigned int) 0;
	this->s = realloc(this->s, 1 * sizeof(this->s[0]));
	if(this->s == NULL) { perror(NULL); exit(EXIT_FAILURE); }

	this->s[0] = '\0';
}

char * vstring_detach(vstring *this)
{
	char *_tmp_1;
	_tmp_1 = this->s;
	this->s = NULL;
	char * retval = (char *) _tmp_1;
	this->len = (unsigned int) 0;
	char *_tmp_3 = (char *) calloc((size_t) (strlen("") + 1u), sizeof(char));
	if(_tmp_3 == NULL) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	char * _tmp_2 = (char *) _tmp_3;
	memcpy(_tmp_2, "", strlen("") + 1u);
	char * _ngg_tmp_1 = _tmp_2;
	if(this->s) {
		free(this->s);
	}

	this->s = _ngg_tmp_1;

	return retval;
}

const char * vstring_get(vstring *this)
{
	return (const char *) this->s;
}

size_t vstring_get_length(vstring *this)
{
	return this->len;
}

void vstring_destruct(vstring *this)
{
	if(this->s) {
		free(this->s);
	}
}

_ngg_tuple_nlg_get_rule nlg_get_rule(NlexHandle *nh)
{
	vstring *vspat = (vstring *) malloc(sizeof(vstring));
	if(vspat == NULL) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	vstring_construct(vspat, "");
	vstring *vsact = (vstring *) malloc(sizeof(vstring));
	if(vsact == NULL) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	vstring_construct(vsact, "");

	_Bool in_list = false;
	_Bool escaped = false;

	while(true) {
		int ch = nlex_next(nh);

		if((((ch == '\r') || (ch == '\n')) || (ch == EOF)) || (ch == '\0')) {
			if(vstring_get_length(vspat) > 0) {
				if(vspat) {
					vstring_destruct(vspat);
					free(vspat);
				}

				if(vsact) {
					vstring_destruct(vsact);
					free(vsact);
				}

				return (_ngg_tuple_nlg_get_rule){NULL, NULL, NLEXERR_NO_ACT_GIVEN};
			} else {
				if(vspat) {
					vstring_destruct(vspat);
					free(vspat);
				}

				if(vsact) {
					vstring_destruct(vsact);
					free(vsact);
				}

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
			while(nlex_next(nh) == ' ') {
			}

			break;
		}

		vstring_appendc(vspat, unwrap_char(ch));
	}

	vstring_appendc(vsact, nlex_last(nh));

	while(true) {
		int ch = nlex_next(nh);
		if((((ch == '\r') || (ch == '\n')) || (ch == EOF)) || (ch == '\0')) {
			break;
		}

		vstring_appendc(vsact, unwrap_char(ch));
	}

	char * pat = vstring_detach(vspat);
	char * act = vstring_detach(vsact);

	if(vspat) {
		vstring_destruct(vspat);
		free(vspat);
	}

	if(vsact) {
		vstring_destruct(vsact);
		free(vsact);
	}

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
			if(pat_autodel) {
				free(pat_autodel);
			}

			return err;
		} else if(!pat) {
			if(pat_autodel) {
				free(pat_autodel);
			}

			return NLEXERR_SUCCESS;
		}

		assert(pat); /* treebuild.ngg:101 */
		assert(act); /* treebuild.ngg:101 */

		err = nlg_tree_add_rule(root, nh, pat, act);
		if(NLEXERR_SUCCESS != err) {
			if(pat_autodel) {
				free(pat_autodel);
			}

			return err;
		}

		if(pat_autodel) {
			free(pat_autodel);
		}
	}

	return NLEXERR_SUCCESS;
}

char unwrap_char(int ch)
{
	assert(ch >= 0 && ch <= 255); /* treebuild.ngg:112 */
	return (char) ch;
}
