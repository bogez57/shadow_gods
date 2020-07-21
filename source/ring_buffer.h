//TODO: don't use member functions
template <typename Type, s64 size>
class StaticRingBuffer
{
    public:
    StaticRingBuffer() = default;
    
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
    
    bool Empty()
    {
        return (NOT this->full && (this->write == this->read));
    };
    
    bool Full()
    {
        return this->full;
    };
    
    void Reset()
    {
        this->write = this->read;
        this->full = false;
    };
    
    s64 Size()
    {
        s64 size = this->maxSize;
        
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
    
    s64 maxSize = size;
    s64 write {};
    s64 read {};
    bool full {};
    Array<Type, size> buffer{};//TODO: Prob need to store on the heap since it's likely this will be used for larger arrays. Need to avoid stack overflow
};