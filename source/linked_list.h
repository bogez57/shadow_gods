#include <list>

template <typename Type>
class Single_List
{
public:
    Single_List() = default;

    void AddLast(const Type& elem)
    {
        this->list.push_back(elem);
    };

    void AddLast(Type&& elem)
    {
        this->list.push_back(elem);
    };

    Type First()
    {
        return this->list.front();
    };

    void PopFirst()
    {
        this->list.pop_front();
    };

    i64 Size()
    {
        return (i64)this->list.size();
    };

private:
    std::list<Type> list;
};