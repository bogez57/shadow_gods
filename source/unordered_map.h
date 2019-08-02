#include <unordered_map>

template <typename Key, typename Value>
class UnorderedMap
{
public:
    UnorderedMap() = default;

private:
    std::unordered_map<Key, Value> map;
};