#pragma once

struct MyNode
{
    void *data;
    MyNode *next;
    MyNode *prev;
};

struct MyList
{
    size_t  size;
    MyNode   *head;
    MyNode   *tail;
};


auto
NewList() -> MyList*;
auto
AddToEnd(MyList* list, void* element) -> void;
auto
GetLastElem(MyList *list, void** out) -> void;