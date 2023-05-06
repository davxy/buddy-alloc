#ifndef _LIST_H_
#define _LIST_H_

#include <stddef.h>

/// Circularly double linked list data structure.
struct list_link {
    /// Previous element.
    struct list_link *prev;
    /// Next element.
    struct list_link *next;
};

/// Initialize a list.
///
/// @param link
///    List link pointer.
static inline void list_link_init(struct list_link *link) {
    link->next = link;
    link->prev = link;
}

/// Insert a new entry between two known consecutive entries.
///
/// This is only for internal list manipulation where we know
/// the prev/next entries already!
///
/// @param prev
/// @param next
/// @param link
static inline void __list_add(
    struct list_link *prev,
    struct list_link *next,
    struct list_link *link
) {
    next->prev = link;
    link->next = next;
    link->prev = prev;
    prev->next = link;
}

/// Insert a new entry after the specified head.
///
/// @param list: list head to add it after
/// @param link: new entry to be added
static inline void list_insert_after(
    struct list_link *list,
    struct list_link *link
) {
    __list_add(list, list->next, link);
}

/// Insert a new entry before the specified head.
///
/// @param new: new entry to be added
/// @param head: list head to add it before
static inline void list_insert_before(
    struct list_link *list,
    struct list_link *link
) {
    __list_add(list->prev, list, link);
}

/// Delete a list entry by making the prev and next entries point to each other.
/// This is only for internal list manipulation where we know the prev and next
/// entries already.
///
/// @param prev
/// @param next
static inline void __list_del(
    struct list_link *prev,
    struct list_link *next
) {
    next->prev = prev;
    prev->next = next;
}

/// Unlink an entry from a list.
///
/// @param elem: the element to delete from the list.
///     After unlinked the element is a single element list.
static inline void list_del(struct list_link *link)
{
    __list_del(link->prev, link->next);
    link->next = link;
    link->prev = link;
}

/// Get the struct start for this list element.
/// @param elem: the struct list_link pointer.
/// @param type: the type of the struct the element is embedded in.
/// @param member: the name of the list_link within the struct.
#define list_container(link, type, member) \
    ((type *) ((char *)(link) - offsetof(type,member)))

/// Tests if the list is empty.
#define list_is_empty(link) \
    ((link)->next == (link))

#endif /* _LIST_H_ */
