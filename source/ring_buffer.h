//TODO: don't use member functions

//TODO: Make this work without dynamic array!!
template <typename Type>
class Ring_Buffer
{
public:
    Ring_Buffer() = default;
    Ring_Buffer(i64 size, i32 memPartitionID_dynamic)
        : maxSize(size)
        , memPartitionID_dynamic(memPartitionID_dynamic)
    {
        this->buffer = CallocType(memPartitionID_dynamic, Type, size);
    };

    void PushBack(Type elem)
    {
        this->buffer[write] = elem;

        if (this->full)
        {
            this->read = (this->read + 1) % this->maxSize;
        }

        this->write = (this->write + 1) % this->maxSize;
        this->full = this->write == this->read;
    };

    Type* GetFirstElem()
    {
        if (this->Empty())
            return nullptr;

        auto* value = &this->buffer[this->read];

        return value;
    };

    Type* GetLastElem()
    {
        if (this->Empty())
            return nullptr;

        return &this->buffer[this->write - 1];
    };

    Type* GetNextElem()
    {
        if(NOT this->Empty() && this->read != this->write && this->Size() > 1)
        {
            if((this->read + 1) == this->maxSize)
                return &this->buffer[0];
            else
                return &this->buffer[this->read + 1];
        }
        else
        {
            return nullptr;
        }
    };

    Type GetFirstElemAndRemove()
    {
        BGZ_ASSERT(NOT this->Empty(), "Trying to access an element from an empty ring buffer container!");

        auto value = this->buffer[this->read];
        this->full = false;
        this->read = (this->read + 1) % maxSize;

        return value;
    };

    void RemoveElem()
    {
        BGZ_ASSERT(NOT this->Empty(), "Trying to remove an element from an empty ring buffer container!");

        this->full = false;
        this->read = (this->read + 1) % this->maxSize;
    };

    b Empty()
    {
        return (NOT this->full && (this->write == this->read));
    };

    b Full()
    {
        return this->full;
    };

    void Reset()
    {
        this->write = this->read;
        this->full = false;
    };

    i64 Size()
    {
        i64 size = this->maxSize;

        if (NOT this->full)
        {
            if (this->write >= this->read)
            {
                size = this->write - this->read;
            }
            else
            {
                size = this->maxSize + this->write - this->read;
            };
        };

        return size;
    };

    void ClearRemaining()
    {
        if(this->read == this->maxSize - 1)
            this->write = 0;
        else
            this->write = this->read + 1;
    };

    i64 maxSize {};
    i32 memPartitionID_dynamic{};
    i64 write {};
    i64 read {};
    b full {};
    Type* buffer { nullptr };
};

template <typename Type>
void Inititialize(Ring_Buffer<Type>&& ringBuff, i64 size, i32 memPartitionID_dynamic)
{
    ringBuff.memPartitionID_dynamic = memPartitionID_dynamic;
    ringBuff.maxSize = size;
    ringBuff.buffer = CallocType(memPartitionID_dynamic, Type, size);
};

template <typename Type>
void CleanUp(Ring_Buffer<Type>&& ringBuff)
{
    DeAlloc(ringBuff.memPartitionID_dynamic, &ringBuff);
};

