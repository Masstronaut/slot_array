// Author: Allan Deutsch
// All content copyright (C) Allan Deutsch 2015. All rights reserved.
#pragma once
#include <utility> // forward
#include <type_traits> // aligned_storage
#include <cstdint> // int64_t, USHRT_MAX
#include <iterator> // iterator
#include <memory> // unique_ptr
#include "../Debugging/Asserts.hpp"
namespace ADL
{
  /*
  * @brief: slot_array is a fixed-size container of linear memory.
  * slot_array makes the following promises:
  *   - Performance
  *     -# contiguous memory
  *     -# O(1) alloc and free operations
  *     -# O(1) access via weak reference keys that can safely attempt to access an element.
  *     -# dereferences have zero overhead when known to be valid\n
  *        ( get() when passed a weak reference key, or via raw pointers)
  *   - Convenience
  *     -# stable indices (a raw pointer to an element will only be invalidated by releasing the container or that object) SEE NOTES
  *     -# freeing an element via iterator will not invalidate the iterator\n
  *        The iterator can no longer be dereferenced, only advanced.
  *
  * @param T The value type being stored in the container
  * @param _Elements The max number of elements the container can hold. Defaults to 2048.
  * @param _Use_Heap Whether element storage should be on the heap or in place. Defaults to on the heap.\n
  *        Specify false when dynamically allocating the slot_array object itself to prevent extra allocations.
  *
  * @note: the structure can not contain more than 64k elements.
  * this is because of bit field usage and the max value of 2^16.
  * 
  * @note: When copied, the elements in the copy will maintain ordering, but weak reference keys will not be valid on the new container.
  * @note: Due to the way some <algorithm> functions work, their usage may invalidate all references/keys/pointers.\n 
  *        Be aware of this when considering using <algorithm> functions with slot_array. 
  */
  template<typename T, unsigned _Elements = 2048u, bool _Use_Heap = true>
  class slot_array
  {
    static_assert(_Elements < UINT16_MAX, "Tried to declare a slot_array with over the maximum capacity (2^16).");
  protected:
    struct element
    {
      element()
        : is_alive(0)
        , counter(0)
        , index(0)
      {
      }

      typename std::aligned_storage<sizeof(T), 4>::type object;
      unsigned is_alive : 1;
      unsigned counter : 15; // unique ID to differentiate current from previous objects allocated in the same slot.
      unsigned index : 16; // stores either the next freelist entry or the allocated element's index.
      unsigned make_ID(unsigned index) const { return ( (is_alive << 31) | (counter << 16) | index ); }
      
    };
  public:
    class iterator;
    using value_type = T;
    using pointer = element*;
    using reference = element&;
    using size_type = short;
    using const_pointer = const pointer;
    using const_reference = const reference;
    using difference_type = ptrdiff_t;
    static const size_t storage_size = sizeof( decltype(element::object) );

    slot_array(); // allocs all the items
    ~slot_array();
    void clear(); // resets data members, destructs any remaining objects
    
    template<typename... Args>
    T& alloc(Args&&... args); // creates an element and returns a reference to it.
    
    iterator erase(iterator position);
    iterator erase(iterator first, iterator last);


    void free(T&);                    // releases the object and puts it on the free list.
    void free(iterator);              // releases the object and puts it on the free list.
    unsigned get_ID(T&)        const; // retreive the ID of the referenced element.
    T& get(unsigned ID)        const; // returns the item referenced by ID
    T* get_safely(unsigned ID) const; // validates the ID before attempting to return the elemnt. nullptr if invalid.
    bool next(T *&)            const; // retrieves the next live item.
    bool previous(T *&)        const; // retrieves the previous live item.

    iterator begin() const;
    const iterator end() const; // May be added in the future to support range-based for loops.

    inline unsigned capacity()    const noexcept { return m_capacity; }
    inline unsigned max_usage()   const noexcept { return m_max_used; }
    inline unsigned size()        const noexcept { return m_size; }
    inline bool empty()           const noexcept { return m_size == 0u; }
    inline float saturation()     const noexcept { return static_cast<float>(m_size) / m_capacity; }
    inline float max_saturation() const noexcept { return static_cast<float>(m_max_used) / m_capacity; }
    
    template<typename Predicate>
    void remove_if(Predicate p);


    class iterator : public std::iterator< std::bidirectional_iterator_tag, T>
    {
      friend class slot_array<T, _Elements, _Use_Heap>;
    public:
      using value_type = T;
      using pointer = T *;
      using reference = T&;
      // constructors
      iterator() = delete;
      iterator(const iterator &rhs);
      iterator(const slot_array &container, T &obj);
      iterator(const slot_array &container, T *obj);
      iterator(const slot_array &container, unsigned ID);
      iterator& operator=(const iterator &rhs);
      iterator& operator++();
      iterator  operator++(int);
      iterator& operator--();
      iterator  operator--(int);


      bool operator==(const iterator &rhs) const;
      bool operator!=(const iterator &rhs) const;

      value_type& operator*() const;
      value_type* operator->() const;

    private:
      iterator(const slot_array &container, element &obj);
      T* m_data;
      const slot_array *m_container;

    };

  private:
    value_type * as_value_type(unsigned index) const;
    

    unsigned get_index(T&) const;

    unsigned m_capacity : 16; // total allocated capacity
    unsigned m_max_used : 16; // max ever active items
    unsigned m_size : 16; // current active items
    unsigned m_free_head : 16; // first free element

    // Use the heap (dynamic allocation) if _Use_Heap is true, otherwise use an array on the stack.
    typename std::conditional<_Use_Heap, std::unique_ptr<element[]>, element[_Elements]>::type m_data;
    //element m_data[_Elements];

    // These two template functions abstract the access of the container data so that it is accessed correctly.
    template< bool dynamic = _Use_Heap >
    void init_storage() { m_data = std::unique_ptr<element[]>(new element[m_capacity]); }
    template<>
    void init_storage<false>() {}
    template< bool dynamic = _Use_Heap >
    const element* data() const { return m_data.get(); }
    template<>
    const element* data<false>() const { return &m_data[0]; }
  };

  

  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::iterator::iterator(const iterator &rhs)
    : m_data(rhs.m_data)
    , m_container(rhs.m_container)
  {}

  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::iterator::iterator(const slot_array &container, element &obj)
    : m_data( reinterpret_cast<T*>(&element.object) )
    , m_container(&container)
  {}
  
  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::iterator::iterator(const slot_array &container, T &obj)
    : m_data( reinterpret_cast<T*>(&obj) )
    , m_container(&container)
  {}

  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::iterator::iterator(const slot_array &container, T *obj)
    : m_data(obj)
    , m_container(&container)
  {}
  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::iterator::iterator(const slot_array &container, unsigned ID)
    : m_data( container.get_safely(ID) )
    , m_container(&container)
  {}

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator& slot_array<T, _Elements, _Use_Heap>::iterator::operator=(const iterator &rhs)
  {
    m_data = rhs.m_data;
    m_container = rhs.m_container;
    return *this;
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator& slot_array<T, _Elements, _Use_Heap>::iterator::operator++()
  {
    m_container->next(m_data);
    return *this;
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator slot_array<T, _Elements, _Use_Heap>::iterator::operator++(int)
  {
    iterator it{ *this };
    m_container->next(m_data);
    return it;
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator& slot_array<T, _Elements, _Use_Heap>::iterator::operator--()
  {
    m_container->previous(m_data);
    return *this;
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator slot_array<T, _Elements, _Use_Heap>::iterator::operator--(int)
  {
    iterator it{ *this };
    m_container->previous(m_data);
    return it;
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  bool slot_array<T, _Elements, _Use_Heap>::iterator::operator==(const iterator& rhs) const
  {
    return ( (m_data == rhs.m_data) && (m_container == rhs.m_container) );
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  bool slot_array<T, _Elements, _Use_Heap>::iterator::operator!=(const iterator& rhs) const
  {
    return ((m_data != rhs.m_data) || (m_container != rhs.m_container));
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::value_type& slot_array<T, _Elements, _Use_Heap>::iterator::operator*() const
  {
    return *m_data;
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::value_type* slot_array<T, _Elements, _Use_Heap>::iterator::operator->() const
  {
    return m_data;
  }

  /*
  * @brief Retrieve the index corresponding to the referenced object.
  * @param object The object for which an index is being acquired.
  * @return the index corresponding to the 'object' param.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  unsigned slot_array<T, _Elements, _Use_Heap>::get_index(T& object) const
  {
    unsigned index;
    //int64_t index{ reinterpret_cast<element*>(&object) - reinterpret_cast<element*>(m_data) };
    if( reinterpret_cast<element*>(&object)->is_alive )
      index = reinterpret_cast<element*>(&object)->index;
    else
    {
      const element* left = reinterpret_cast<element*>(&object);
      const element* right = data();
      index = static_cast<unsigned>( left - right );
    }
    ADL_ASSERT_MSG( ( index < m_capacity ) && ( index >= 0 ), "Tried to free an element that was out of bounds.");
    //return static_cast<unsigned>(index);
    return index;
  }

  /*
  * @brief Casts an element index to a pointer to an object of the value_type type stored at that index.
  * @param index The index of the object to retrieve
  * @return Pointer to the object stored at the index referenced by the 'index' param.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  T* slot_array<T, _Elements, _Use_Heap>::as_value_type(unsigned index) const
  { 
    return reinterpret_cast<value_type *>( const_cast<std::aligned_storage<sizeof(T), 4>::type *>(&m_data[index].object) );
  }

  /*
  * @brief Initializes all member variables and the free list.
  * @detail If _Use_Heap is true it will allocate using the heap, 
  * otherwise the elements will be stored in place in the slot_array.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::slot_array()
    : m_capacity(_Elements)
    , m_max_used(0)
    , m_size(0)
    , m_free_head(0)
  {
    init_storage();
    
    // Setup the in place free list
    for (unsigned i{ 0 }; i < m_capacity; ++i)
    {
      m_data[i].index = i + 1;
    }
  }

  /*
  * @brief destructs all live objects and releases any memory it allocated.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  slot_array<T, _Elements, _Use_Heap>::~slot_array()
  {
    clear();
  }

  /*
  * @brief Releases all live elements and sets up the free list.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  void slot_array<T, _Elements, _Use_Heap>::clear()
  {
    for (unsigned i{ 0 }; i < m_capacity; ++i)
    {
      if (m_data[i].is_alive)
      {
        free( *reinterpret_cast<T*>( &m_data[i] ) );
      }
      
      m_data[i].index = i + 1;
    }

    ADL_ASSERT(m_size == 0);
    m_free_head = 0;
  }

  /*
  * @brief Allocates an object in the slot_array and returns a reference to the object.
  * @param args Any arguments passed to the function will be forwarded to the constructor of the value_type.
  * @return A reference to the allocated object.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap> 
  template<typename... Args>
  T& slot_array<T, _Elements, _Use_Heap>::alloc(Args&&... args)
  {
    ADL_ASSERT_MSG(m_size < m_capacity, "Tried to allocate in a fully saturated container.");
    element& elem{ m_data[m_free_head] };
    ADL_ASSERT_MSG(elem.is_alive == false, "Tried to allocate in an element that is already in use.");
    
    unsigned temp = elem.index;
    // The element stores it's own index if it is live
    elem.index = m_free_head; 
    // remove the element from the free list
    m_free_head = temp;

    T* data = new(&elem.object) T( std::forward<Args>(args)... );
    
    elem.is_alive = true;
    ++m_size;
    // set the max used if we have a new record;
    m_max_used = std::max(m_max_used, m_size);
    return *as_value_type(elem.index);

  }

  /*
  * @brief Accesses an element of the slot_array by key. Only use this if the key is known to be valid.
  * @param ID An identifier known to be good that references an object in the slot_array
  * @return A reference to the object corresponding to the ID.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  T& slot_array<T, _Elements, _Use_Heap>::get(unsigned ID) const
  {
    // Acquire the 16 higher bits of the ID
    // This isn't done in this get implementation, because it is assumed to be a valid ID.
    //unsigned key{ ((ID >> 16) && 0xFFFF) };

    // Acquire the 16 lower bits of the ID
    unsigned index{ ID & 0x0000FFFF };
    ADL_ASSERT_MSG(index < _Elements, "Invalid ID: Index out of range.");
    return *as_value_type(index);

  }

  /*
  * @brief Accesses an element of the slot_array by key safely and can be used if the key isn't known to be valid.
  * @param ID An identifier that references an object in the slot_array
  * @return A pointer to the object corresponding to the ID. If the ID is invalid, it will return nullptr.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  T* slot_array<T, _Elements, _Use_Heap>::get_safely(unsigned ID) const
  {
    // Acquire bits 16-30 (zero indexed) of the ID
    unsigned key{ ( ID >> 16 ) & 0x7FFF };
    //                            ^ 0111 1111 1111 1111

    // Acquire the 16 lower bits of the ID
    unsigned index{ ID & 0xFFFF };
    ADL_ASSERT_MSG(index < _Elements, "Invalid ID: Index out of range.");
    element& object = m_data[index];

    if(object.counter == key )
    {
      return  as_value_type(object.index);
    }
    else
    {
      return nullptr;
    }
  }

  /*
  * @brief Generates a unique identifier used to safely access the referenced object.
  * @param object The object who's ID is being acquired.
  * @return An ID that can be used to safely access the object.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  unsigned slot_array<T, _Elements, _Use_Heap>::get_ID(T& object) const
  {
    unsigned index = get_index(object);

    return m_data[index].make_ID(index);
  }

  /*
  * @brief Releases an element from the slot_array and makes that slot available.
  * @param position The position of the element to erase.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator slot_array<T, _Elements, _Use_Heap>::erase(iterator position)
  {
    ADL_ASSERT_MSG(this == position.m_container, "Iterator/container mismatched.");
    free(*position);
    return ++position;
  }

  /*
  * @brief Erases a range of elements [first, last) from teh slot_array and makes those slots available.
  * @param first The start of the range to erase.
  * @param last One past the end of the range to remove.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator slot_array<T, _Elements, _Use_Heap>::erase(iterator first, iterator last)
  {
    ADL_ASSERT_MSG(this == first.m_container, "Iterator/container mismatched.");
    ADL_ASSERT_MSG(this == last.m_container, "Iterator/container mismatched.");
    for (; first != last; ++first)
    {
      free(*first);
    }
    return first;
  }

  /*
  * @brief Safely destructs the referenced object and makes it's slot available for new objects.
  * @param object The object to be freed.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  void slot_array<T, _Elements, _Use_Heap>::free(T& object)
  {
    unsigned index{ get_index(object) };
    element& elem{ m_data[ index ] };
    ADL_ASSERT_MSG(elem.is_alive, "Tried to free an object that wasn't alive.");
    reinterpret_cast<T*>(&elem.object)->~T();
    ++elem.counter;
    --m_size;
    elem.is_alive = false;
    // Add this element to the front of the free list
    elem.index = m_free_head;
    m_free_head = index;
  }

  /*
  * @brief Safely destructs the referenced object and makes it's slot available for new objects.
  * @param object The object to be freed.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  void slot_array<T, _Elements, _Use_Heap>::free(iterator object)
  {
    free(*object.m_data);
  }

  /*
  * @brief Used to iterate over the live elements of the slot_array.
  * @param object A pointer reference that will be set to point to the next element. 
                  Will be nullptr if none remain. Pass in a nullptr to start with the first element.
  * @return Returns whether the pointer is valid or not. Convenient for use in a while loop.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  bool slot_array<T, _Elements, _Use_Heap>::next(T*& object) const
  {
    if (m_size == 0u)
    {
      object = nullptr;
      return false;
    }

    // General case: valid object pointer and we start from one past that object's index.
    if (object)
    {
      unsigned index{ get_index(*object) };
      for (unsigned i{ index + 1 }; i < m_capacity; ++i)
      {
        if (m_data[i].is_alive)
        {
          object = as_value_type(i);
          return true;
        }
      }
    }
    else // Handle the case where a nullptr is passed in and we start from the first element
    {
      for (unsigned i{ 0 }; i < m_capacity; ++i)
      {
        if (m_data[i].is_alive)
        {
          object = as_value_type(i);
          return true;
        }
      }
    }


    object = nullptr;
    return false;
  }


  /*
  * @brief Used to iterate over the live elements of the slot_array.
  * @param object A pointer reference that will be set to point to the next element.
  Will be nullptr if none remain. Pass in a nullptr to start with the first element.
  * @return Returns whether the pointer is valid or not. Convenient for use in a while loop.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  bool slot_array<T, _Elements, _Use_Heap>::previous(T*& object) const
  {
    if (m_size == 0u)
    {
      object = nullptr;
      return false;
    }

    // General case: valid object pointer and we start from one past that object's index.
    if (object)
    {
      unsigned index{ get_index(*object) };
      //                     v subtracting 1 because the data starts at the 0 index
      for (unsigned i{ index - 1 }; i != UINT32_MAX; --i) // UINT32_MAX is the result of decrementing 0.
      {
        if (m_data[i].is_alive)
        {
          object = as_value_type(i);
          return true;
        }
      }
    }
    else // Handle the case where a nullptr is passed in and we start from the first element
    {
      //                         v subtracting 1 because the data starts at the 0 index
      for (unsigned i{ _Elements - 1 }; i != UINT32_MAX; --i) // UINT32_MAX is the result of decrementing 0.
      {
        if (m_data[i].is_alive)
        {
          object = as_value_type(i);
          return true;
        }
      }
    }


    object = nullptr;
    return false;
  }

  /*
  * @brief Access the first element of the slot_array. Can be used in conjunction with next() to iterate over all elements.
  * @return An iterator with the first live element of the slot_array.
  */
  template<typename T, unsigned _Elements, bool _Use_Heap>
  typename slot_array<T, _Elements, _Use_Heap>::iterator slot_array<T, _Elements, _Use_Heap>::begin() const noexcept
  {
    if (m_size == 0u)
    {
      return end();
    }
    for (unsigned i{ 0u }; i < _Elements; ++i)
    {
      if (m_data[i].is_alive)
      {
        return iterator( *this, as_value_type(i) );
      }
    }

    // This can only occur when an element seems to exist, but could not be found within the bounds of the container.
    ADL_ASSERT("An internal error has occurred.");
    return iterator{ *this, nullptr };
  }
   
  template<typename T, unsigned _Elements, bool _Use_Heap>
  const typename slot_array<T, _Elements, _Use_Heap>::iterator slot_array<T, _Elements, _Use_Heap>::end() const noexcept
  {
    return iterator{ *this, nullptr };
  }

  template<typename T, unsigned _Elements, bool _Use_Heap>
  template<typename Predicate>
  void slot_array<T, _Elements, _Use_Heap>::remove_if(Predicate p)
  {
    for (auto & it : *this)
    {
      if (p(it))
      {
        free(it);
      }
    }
  }

}
