#ifndef _LNLY_LIST_
#define _LNLY_LIST_

struct list {
	struct list_item *list;
};

struct list_item {
	struct list_item *next;
	void *data;
};
	

/* return:
 * 1 if a > b
 * 0 if a == b
 * -1 if a < b
 */
typedef int (*compare_func_t)(const void *a, const void *b);
typedef void (*list_pass_cb)(struct list_item *item, void *data);
typedef void (*data_destroy_func_t)(void *a);



struct list *list_init();

/* create new list item */ 
struct list_item *list_item_new(const void *data);

/* add new item in list */
void list_item_add(struct list *root, struct list_item *item);

/* add new item in list(to end of list */
void list_item_add_to_end(struct list *root, struct list_item *item);

/* return first element of list, NULL otherwise */
struct list_item *list_item_get_first(struct list *root);

/* return n element of list, NULL otherwise */
struct list_item *list_item_get_n(struct list *root, unsigned int n);


void list_reverse(struct list *root);

/* Destroy list with all list_item */
void list_destroy(struct list **root, data_destroy_func_t func);


//list find return ptr if finds, NULL otherwise
struct list_item *list_find(const struct list *root, const void *data, compare_func_t func);


/*
 * Pass list. return list len
 * If list_pass_cb != 0 then list_pass_cb will be implement at every element of list
 */
int list_pass(struct list *root, list_pass_cb func, void *data);

//
int list_sort(struct list *root, compare_func_t func);

int list_sort_simple(struct list *lst, compare_func_t func);

#endif

