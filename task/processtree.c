#include "processtree.h"
#include "process.h"
#include <mem/heap.h>
#include <util/util.h>
#include <terminal/vga.h>

void setup_process_tree(void)
{
	// Assumes that current_process is valid
	process_tree=kmalloc(sizeof(struct pnode));
	struct pnode *root=(struct pnode*)process_tree;
	root->process = (struct process*)current_process;
	root->first_child=NULL;
	root->parent=NULL;
	root->next=NULL;
	root->prev=NULL;

	print_startup_info("Process tree", true);
}

static struct pnode *find_process_from_children(const struct process *needle, struct pnode *p)
{
	if(!p || !p->first_child) return NULL;
	struct pnode *node=p->first_child;
	while(node)
	{
		if(node->process==needle) return node;
		else node=node->next;
	}
	return NULL;
}

struct pnode* find_process_from_process_tree(const struct process *p)
{
	struct pnode *root=(struct pnode*)process_tree;
	struct pnode *node=root;
	if(node->process == p) return node;
	while(node)
	{
		struct pnode *result=find_process_from_children(p, node);
		if(result) return result;
		else node=node->first_child;
	}
	return NULL;
}

int insert_process_to_process_tree(struct process *p, struct process *parent)
{
	struct pnode *parentnode=find_process_from_process_tree(parent);
	if(!parentnode) PANIC();
	struct pnode *node=parentnode->first_child;
	if(!node)
	{
		node=kmalloc(sizeof(struct pnode));
		node->process=p;
		node->parent=(struct pnode*)parentnode;
		node->first_child=NULL;
		node->next=NULL;
		node->prev=NULL;
		parentnode->first_child=node;
		return 0;
	}
	while(node->next) node=node->next;
	node->next=kmalloc(sizeof(struct pnode));
	node->next->parent=parentnode;
	node->next->process=p;
	node->next->first_child=NULL;
	node->next->next=NULL;
	node->next->prev=node;
	return 0;
}

int delete_process_from_process_tree(const struct process *p)
{
	struct pnode *node=find_process_from_process_tree(p);
	if(!node) return -1;
	if(node->prev) node->prev->next=node->next;
	else node->parent->first_child=NULL;
	// TODO: Free node
	return 0;
}
