#ifndef _TAPIOS_LIST_H_
#define _TAPIOS_LIST_H_

struct list
{
	struct list *next;
};

#define list_get(listptr, structname) (void*)((char*)listptr - offsetof(structname, list))
#define list_foreach(listptr, structname, current) if(listptr) for(structname *current=listptr; current; current = (current->list.next ? list_get(current->list.next, structname) : NULL))
#define list_add(listptr, elem) {if(!listptr) listptr=elem; else _list_add(&listptr->list, &elem->list);}
void _list_add(struct list *list, struct list *elem);

#endif
