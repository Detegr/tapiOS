#ifndef _TAPIOS_LIST_H_
#define _TAPIOS_LIST_H_

struct list
{
	struct list *next;
};

#define list_get(listptr, structname) (void*)((char*)listptr - offsetof(structname, list))
#define list_foreach(listptr, structname, current) for(structname *current=listptr; current->list.next; current=list_get(current->list.next, structname))
#define list_add(listptr, elem) _list_add(&listptr->list, &elem->list);
void _list_add(struct list *list, struct list *elem);

#endif
