#include "jsi.h"
#include "utf.h"

/* Use an AA-tree to quickly look up interned strings. */

js_StringNode jsS_sentinel = { &jsS_sentinel, &jsS_sentinel, 0, 0, 0, 0, ""};

static js_StringNode *jsS_newstringnode(js_State *J, const char *string, const char **result)
{
	unsigned int n = 0;
	unsigned int len = utflen2(string, &n);
	js_StringNode *node = js_malloc(J, soffsetof(js_StringNode, string) + n + 1);
	node->left = node->right = &jsS_sentinel;
	node->level = 1;
	node->size = n;
	node->length = len;
	node->isunicode = n != len;
	memcpy(node->string, string, n + 1);
	return *result = node->string, node;
}

static js_StringNode *jsS_skew(js_StringNode *node)
{
	if (node->left->level == node->level) {
		js_StringNode *temp = node;
		node = node->left;
		temp->left = node->right;
		node->right = temp;
	}
	return node;
}

static js_StringNode *jsS_split(js_StringNode *node)
{
	if (node->right->right->level == node->level) {
		js_StringNode *temp = node;
		node = node->right;
		temp->right = node->left;
		node->left = temp;
		++node->level;
	}
	return node;
}

static js_StringNode *jsS_insert(js_State *J, js_StringNode *node, const char *string, const char **result)
{
	if (node != &jsS_sentinel) {
		int c = strcmp(string, node->string);
		if (c < 0)
			node->left = jsS_insert(J, node->left, string, result);
		else if (c > 0)
			node->right = jsS_insert(J, node->right, string, result);
		else
			return *result = node->string, node;
		node = jsS_skew(node);
		node = jsS_split(node);
		return node;
	}
	return jsS_newstringnode(J, string, result);
}

static void dumpstringnode(js_StringNode *node, int level)
{
	int i;
	if (node->left != &jsS_sentinel)
		dumpstringnode(node->left, level + 1);
	printf("%d: ", node->level);
	for (i = 0; i < level; ++i)
		putchar('\t');
	printf("'%s'\n", node->string);
	if (node->right != &jsS_sentinel)
		dumpstringnode(node->right, level + 1);
}

void jsS_dumpstrings(js_State *J)
{
	js_StringNode *root = J->strings;
	printf("interned strings {\n");
	if (root && root != &jsS_sentinel)
		dumpstringnode(root, 1);
	printf("}\n");
}

static void jsS_freestringnode(js_State *J, js_StringNode *node)
{
	if (node->left != &jsS_sentinel) jsS_freestringnode(J, node->left);
	if (node->right != &jsS_sentinel) jsS_freestringnode(J, node->right);
	js_free(J, node);
}

void jsS_freestrings(js_State *J)
{
	if (J->strings && J->strings != &jsS_sentinel)
		jsS_freestringnode(J, J->strings);
}

const char *js_intern(js_State *J, const char *s)
{
	const char *result;
	if (!J->strings)
		J->strings = &jsS_sentinel;
	J->strings = jsS_insert(J, J->strings, s, &result);
	return result;
}
