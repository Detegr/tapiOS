#ifndef _TAPIOS_PROCESSTREE_H_
#define _TAPIOS_PROCESSTREE_H_

struct pnode
{
	struct process *process;
	struct pnode *parent;
	struct pnode *first_child;
	struct pnode *next;
	struct pnode *prev;
};

volatile struct pnode *process_tree;

void setup_process_tree(void);
int insert_process_to_process_tree(struct process *p, struct process *parent);
struct pnode *find_process_from_process_tree(const struct process *p);
int delete_process_from_process_tree(const struct process *p);

#endif
