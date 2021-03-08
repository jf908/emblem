#include "list.h"

#include "stddef.h"

void make_list(List* l)
{
	l->fst = NULL;
	l->lst = NULL;
	l->cnt = 0;
}

void dest_list(List* l, bool freeNodes, Destructor ed)
{
	if (freeNodes || ed)
	{
		ListNode* curr = l->fst;
		while (curr)
		{
			ListNode* nxt = curr->nxt;
			dest_list_node(curr, ed);

			if (freeNodes)
				free(curr);

			curr = nxt;
		}
	}
}

void make_list_node(ListNode* ln, void* data)
{
	ln->nxt	 = NULL;
	ln->prv	 = NULL;
	ln->data = data;
}

void dest_list_node(ListNode* ln, Destructor ed)
{
	if (ed)
		ed(ln->data);
}

bool append_list_node(List* l, ListNode* ln)
{
	if (!l || !ln)
		return false;

	ln->prv = l->lst;
	ln->nxt = NULL;

	if (!l->fst)
		l->fst = ln;

	if (l->lst)
		l->lst->nxt = ln;
	l->lst = ln;

	l->cnt++;

	return true;
}

bool prepend_list_node(List* l, ListNode* ln)
{
	if (!l || !ln)
		return false;

	ln->nxt = l->fst;
	ln->prv = NULL;

	if (l->fst)
		l->fst->prv = ln;
	l->fst = ln;

	if (!l->lst)
		l->lst = ln;

	l->cnt++;

	return true;
}

bool remove_list_node(List* l, ListNode* ln)
{
	if (!l || !ln)
		return false;

	if (ln->nxt)
		ln->nxt->prv = ln->prv;
	if (ln->prv)
		ln->prv->nxt = ln->nxt;

	if (l->fst == ln)
		l->fst = ln->nxt;

	if (l->lst == ln)
		l->lst = ln->prv;

	ln->nxt = NULL;
	ln->prv = NULL;

	l->cnt--;

	return true;
}

void make_list_iter(ListIter* i, List* l) { i->nxt = l->fst; }

void dest_list_iter(ListIter* i __attribute__((unused))) { }

bool iter_list(void** val, ListIter* i)
{
	bool succ = i->nxt;
	if (succ)
	{
		*val   = i->nxt->data;
		i->nxt = i->nxt->nxt;
	}
	else
		*val = NULL;

	return !!succ;
}

void make_list_from_arr(List* l, Array* arr)
{
	make_list(l);
	l->cnt			   = arr->cnt;
	ListNode* prevNode = NULL;
	for (size_t i = 0; i < arr->cnt; i++)
	{
		ListNode* ln = malloc(sizeof(ListNode));
		make_list_node(ln, arr->data[i]);

		if (!i)
			l->fst = ln;
		if (i == l->cnt - 1)
			l->lst = ln;

		if (prevNode)
			prevNode->nxt = ln;
		ln->prv = prevNode;
		prevNode	  = ln;
	}
}

void in_list(Maybe* m, List* l, void* val) { in_list_eq(m, l, cmp_size_ts, val); }

void in_list_eq(Maybe* m, List* l, Comparator cmp, void* val)
{
	ListNode* curr = l->fst;

	while (curr)
	{
		if (!cmp(val, curr->data))
		{
			break;
		}
		curr = curr->nxt;
	}

	if (curr)
		make_maybe_just(m, curr);
	else
		make_maybe_nothing(m);
}

bool all_list(List* l)
{
	if (!l)
		return true;

	bool r		   = true;
	ListNode* curr = l->fst;

	while (curr)
	{
		r &= !!curr->data;
		curr = curr->nxt;
	}

	return r;
}

bool any_list(List* l)
{
	if (!l)
		return false;

	bool r		   = false;
	ListNode* curr = l->fst;

	while (curr)
	{
		r |= !!curr->data;
		curr = curr->nxt;
	}

	return r;
}

bool is_empty_list(List* l)
{
	return l->cnt == 0;
}
