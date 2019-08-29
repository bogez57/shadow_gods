
template <typename Type>
class Ring_Buffer
{
public:
    Ring_Buffer() = default;
    Ring_Buffer(i64 size, i32 memPartitionID_dynamic)
        : maxSize(size)
    {
        this->buffer = MallocType(memPartitionID_dynamic, Type, size);
    };

    void Init(i64 size, i32 memPartitionID_dynamic)
    {
        this->maxSize = size;
        this->buffer = MallocType(memPartitionID_dynamic, Type, size);
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

    Type* GetFirstElem()
    {
        if (this->Empty())
            return nullptr; 

        auto* value = &this->buffer[this->tail];

        return value;
    };

    Type* GetLastElem()
    {
        if (this->Empty())
            return nullptr; 

        return &this->buffer[this->head];
    };

    Type GetFirstElemAndRemove()
    {

        BGZ_ASSERT(NOT this->Empty(), "Trying to access an element from an empty ring buffer container!");

        auto value = this->buffer[this->tail];
        this->full = false;
        this->tail = (this->tail + 1) % maxSize;

        return value;
    };

    void RemoveElem()
    {
        BGZ_ASSERT(NOT this->Empty(), "Trying to remove an element from an empty ring buffer container!");

        this->full = false;
        this->tail = (this->tail + 1) % this->maxSize;
    };

    b Empty()
    {
        return (!this->full && (this->head == this->tail));
    };

    b Full()
    {
        return this->full;
    };

    void Reset()
    {
        this->head = this->tail;
        this->full = false;
    };

    i64 Size()
    {
        i64 size = this->maxSize;

        if (NOT this->full)
        {
            if (this->head >= this->tail)
            {
                size = this->head - this->tail;
            }
            else
            {
                size = this->maxSize + this->head - this->tail;
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