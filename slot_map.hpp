
#include <vector>
#include <tuple>



template<typename Value, typename Token = std::pair<unsigned, unsigned>, template<typename>typename Container = std::vector>
class slot_map{
  using key_size_type = decltype(std::get<0>(std::declval<Token>()));
  using key_generation_type = decltype(std::get<1>(std::declval<Token>()));
// constraint enforcement
static_assert(sizeof(key_size_type) <= sizeof(Container::size_type)
, "The size of the type of get<0>() token must be <= Container::size_type.");


  public:
using mapped_type = typename Container::value_type;
using value_type = mapped_type;
using key_type = Token;
using size_type = decltype(std::get<0>(std::declval<Token>()));
using reference = typename Container::reference;
using const_reference = typename Container::const_reference;
using pointer = typename Container::pointer;
using const_pointer = typename Container::const_pointer;
using container_type = Container;

// Note: 
// slot_map requires a container providing an iterator satisfying the constraints of RandomAccessIterator
using iterator = typename Container::iterator;
using const_iterator = typename Container::const_iterator;
using reverse_iterator = typename Container::reverse_iterator;
using const_reverse_iterator = typename Container::const_reverse_iterator;

// Constructors
// Destructors
// assignment operators

// Element access (guaranteed O(1) ). 
// Note that these return a pointer to avoid requiring they default construct an object if none is found.
pointer at(const key_type& key);                // bounds+generation checked, O(1) in all cases
const_pointer at(const key_type& key) const;    // bounds+generation checked, O(1) in all cases
pointer operator[](const key_type& key);        // generation checked, O(1) in all cases
pointer operator[](key_type&& key);             // generation checked, O(1) in all cases
iterator find(const key_type& key);             // generation checked, O(1) in all cases
const_iterator find(const key_type& key) const; // generation checked, O(1) in all cases

// Element access completely unique to slot_map
// These functions could be considered "unsafe" but they offer 
// a lookup with no branching for users confident they have a valid key
reference find_unchecked(const key_type& key);             // unsafe, no checks, O(1) in all cases
const_reference find_unchecked(const key_type& key) const; // unsafe, no checks, O(1) in all cases




// Iterators
iterator begin();                         // O(1) in all cases
iterator end();                           // O(1) in all cases
const_iterator begin() const;             // O(1) in all cases
const_iterator end() const;               // O(1) in all cases
const_iterator cbegin() const;            // O(1) in all cases
const_iterator cend() const;              // O(1) in all cases
reverse_iterator rbegin();                // O(1) in all cases
reverse_iterator rend();                  // O(1) in all cases
const_reverse_iterator rbegin() const;    // O(1) in all cases
const_reverse_iterator rend() const;      // O(1) in all cases
const_reverse_iterator crbegin() const;   // O(1) in all cases
const_reverse_iterator crend() const;     // O(1) in all cases

// Capacity
bool empty() const;         // O(1) in all cases
size_type size() const;     // O(1) in all cases
size_type max_size() const; // O(1) in all cases
size_type capacity() const; // O(1) in all cases
void reserve(size_type n);  // O(N), O(1) if(empty() && n<slot_capacity() ), or if(n < capacity())

// Capacity unique to slot_map
void reserve_slots(size_type n); // O(N), O(1) if(capacity_slots() > n)
size_type capacity_slots() const; // O(1) in all cases

// Modifiers equivalent to map
void clear();                                               // O(N)
key_type insert(const_reference value);                     // O(1), O(N) when allocation is required
key_type insert(value_type&& value);                        // O(1), O(N) when allocation is required
template<typename... Args>
key_type emplace(Args&&... args);                           // O(1), O(N) when allocation is required
iterator erase(iterator pos);                               // O(1) in all cases
iterator erase(iterator first, iterator last);              // O(1) per element being erased, worst case O(N)
iterator erase(const_iterator pos);                         // O(1) in all cases
iterator erase(const_iterator first, const_iterator last);  // O(1) per element being erased, worst case O(N)
std::optional<iterator> erase(const key_type& key);         // O(1) in all cases. Will contain an iterator if an element was erased

// Modifiers unique to slot_map
std::optional<key_type> insert_at(const_reference value, size_type slot); // O(1) in all cases. Attempts to insert at the specified slot.
std::optional<key_type> insert_at(value_type&& value, size_type slot);    // O(1) in all cases. Attempts to insert at the specified slot.
// This should be okay given the precedent for it using an iterator set by std::set::emplace_hint
template<typename... Args>
std::optional<key_type> emplace_at(size_type slot, Args&&... args);       // O(1) in all cases. Attempts to emplace at the specified slot.


private:
Container<key_type> m_keys;
Container<Value>\ m_data;
// this uses memory to offer O(1) for erase() overloads other than erase(const key_type&)
Container<decltype(std::get<0>(key_type))> m_erase_helper; 
decltype(std::get<0>(key_type)) m_free_head;
decltype(std::get<0>(key_type)) m_free_tail;

size_type pop_head();
void push_tail(size_type);
key_type finish_inserting_last_element();

};  

















template<typename Value, typename Token, template<typename> typename Container>
slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::finish_inserting_last_element()){
  size_type key_index{pop_head()};
  key_type& key{m_keys[key_index]};
  auto& [index, generation] = key;
  index = m_data.size() - 1;
  m_erase_helper.push_back(key_index);
  return { decltype(std::decay_t<decltype(index)>)(key_index), generation };
}

template<typename Value, typename Token, template<typename> typename Container>
slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::insert(const_reference value){
  m_data.push_back(value);
  return finish_inserting_last_element();
}

template<typename Value, typename Token, template<typename> typename Container>
slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::insert(value_type&& value){
  m_data.push_back(std::move(value));
  return finish_inserting_last_element();
}

template<typename Value, typename Token, template<typename> typename Container>
template<typename... Args>
slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::emplace(Args&&... args){
  m_data.emplace_back(std::forward<Args>(args)...);
  return finish_inserting_last_element();
}


// begins


/*
Questions:

Is leaving the overflow for generation counter up to the user provided type fine? (default is do nothing)
Yes

Discussion: accessing the underlying container. Desirable, but can break the container if insert or erase happens

- address substitutability with map and unordered_map

- email nico josuttis for help with naming and interface feedback

- add small_vector and static_vector as desirable underlying container types, thus adapter

- need examples




*/