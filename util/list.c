#include "list.h"
#include <stdlib.h>

void _list_add(struct list *list, struct list *elem)
{
	struct list *l=list;
	while(l->next) l=l->next;
	l->next=elem;
	elem->next=NULL;
}
