
auto
NewList() -> MyList*
{
    MyList* List = PushType(MyList, 1);

    return List;
};

auto
AddToEnd(MyList* list, void* element) -> void
{
    MyNode* node = PushType(MyNode, 1);

    node->data = element;

    if (list->size == 0) 
    {
        list->head = node;
        list->tail = node;
    } 
    else 
    {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    };

    list->size++;
};

auto
GetLastElem(MyList *list, void** out) -> void
{
    if (list->size == 0)
        BGZ_ASSERT(1 == 0);

    *out = list->tail->data;
};

auto
RemoveLastElem(MyList* list) -> void
{
    if (!list->size)
        BGZ_ASSERT(1 == 0);

    MyNode* node = list->tail; 
    void *data = node->data;

    BGZ_ASSERT(node->next != NULL);
    list->tail = node->prev;

    PopSize(sizeof(MyNode));
    list->size--;
};