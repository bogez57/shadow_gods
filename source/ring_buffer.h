
template <typename Type>
class Ring_Buffer
{
public:
    Ring_Buffer() = default;
    Ring_Buffer(i64 size, i32 memPartitionID_dynamic)
        : maxSize(size)
        , allocator(allocator)

    {
        this->buffer = (Type*)MallocSize(memPartitionID_dynamic, size);
    };

    void Init(i64 size, i32 memPartitionID_dynamic)
    {
        this->allocator = allocator;
        this->maxSize = size;
        this->buffer = (Type*)MallocSize(memPartitionID_dynamic, size);
    };

    void PushBack(Type elem)
    {
        this->buffer[head] = elem;

        if (this->full)
        {
            this->tail = (this->tail + 1) % this->maxSize;
        }

        this->head = (this->head + 1) % this->maxSize;
        this->full = this->head == this->tail;
    };

    Type GetFirstElem()
    {
        if (this->Empty())
        {
            return Type();
        }

        auto value = buffer[tail];

        return value;
    };

    Type GetFirstElemAndRemove()
    {

        BGZ_ASSERT(NOT this->Empty(), "Trying to access an element from an empty ring buffer container!");

        auto value = buffer[tail];
        full = false;
        tail = (tail + 1) % maxSize;

        return value;
    };

    void RemoveElem()
    {
        BGZ_ASSERT(NOT this->Empty(), "Trying to remove an element from an empty ring buffer container!");

        full = false;
        tail = (tail + 1) % maxSize;
    };

    b Empty()
    {
        return (!full && (head == tail));
    };

    b Full()
    {
        return full;
    };

    void Reset()
    {
        head = tail;
        full = false;
    };

    i64 Size()
    {
        i64 size = maxSize;

        if (NOT full)
        {
            if (head >= tail)
            {
                size = head - tail;
            }
            else
            {
                size = maxSize + head - tail;
            };
        };

        return size;
    };

    i64 maxSize {};
    i64 head {};
    i64 tail {};
    b full {};
    Type* buffer { nullptr };
};