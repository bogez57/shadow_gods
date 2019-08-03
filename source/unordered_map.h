#include <map>

template <typename Key, typename Value>
class UnorderedMap
{
public:
    UnorderedMap() = default;
    UnorderedMap(Key key, Value val = {})
    {
        map.insert(std::pair<Key, Value>(key, val));
    };

    void Insert(Key key, Value val = {})
    {
        map.insert(std::pair<Key, Value>(key, val));
    };

    Value At(Key key)
    {
        return map.at(key);
    };

private:
    std::map<Key, Value> map;
};