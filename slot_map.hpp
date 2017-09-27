
#include <vector>
#include <tuple>
#include <optional>
#include <limits> // find max value of key_size_type
#include <type_traits> // declval, remove_reference


template<typename Value, typename Token = std::pair<unsigned, unsigned>, template<typename...>typename Container = std::vector>
class slot_map {
public:
  using key_size_type = std::remove_reference_t<decltype( std::get<0>( std::declval<Token>( ) ) )>;
  using key_generation_type = std::remove_reference_t<decltype( std::get<1>( std::declval<Token>( ) ) )>;
  using mapped_type = typename Container<Value>::value_type;
  using value_type = mapped_type;
  using key_type = Token;
  using size_type = std::remove_reference_t<decltype( std::get<0>( std::declval<Token>( ) ) )>;
  using reference = typename Container<value_type>::reference;
  using const_reference = typename Container<value_type>::const_reference;
  using pointer = typename Container<value_type>::pointer;
  using const_pointer = typename Container<value_type>::const_pointer;
  using container_type = Container<value_type>;

  // Note: 
  // slot_map requires a container providing an iterator satisfying the constraints of RandomAccessIterator
  using iterator = typename Container<value_type>::iterator;
  using const_iterator = typename Container<value_type>::const_iterator;
  using reverse_iterator = typename Container<value_type>::reverse_iterator;
  using const_reverse_iterator = typename Container<value_type>::const_reverse_iterator;


  // constraint enforcement
  static_assert( sizeof( key_size_type ) <= sizeof( typename Container<Value>::size_type )
                 , "The size of the type of get<0>() for the Token type must be at most Container::size_type." );
  static_assert( std::tuple_size<Token>::value == 2, "The token type for a slot_map must be decomposable into 2 integer-like types." );
  static_assert( std::is_same_v<typename std::iterator_traits<iterator>::iterator_category, std::random_access_iterator_tag>, 
                 "slot_map requires the adapted container to provide random access iterators." );



  // Constructors
  constexpr slot_map( );
  constexpr slot_map( const slot_map & ) = default;
  constexpr slot_map( slot_map && ) = default;
  // Destructors
  ~slot_map( ) = default;
  // assignment operators
  constexpr slot_map& operator=( const slot_map & ) = default;
  constexpr slot_map& operator=( slot_map && ) = default;
  
  constexpr const container_type& data( ) const;

  // Element access (guaranteed O(1) ). 
  // Note that these return a pointer to avoid requiring they default construct an object if none is found.
  constexpr pointer at( const key_type& key );                           // bounds+generation checked, O(1) in all cases
  constexpr const_pointer at( const key_type& key ) const;               // bounds+generation checked, O(1) in all cases
  constexpr pointer operator[]( const key_type& key );                   // generation checked, O(1) in all cases
  constexpr const_pointer operator[]( const key_type& key ) const;       // generation checked, O(1) in all cases
  constexpr iterator find( const key_type& key );                        // generation checked, O(1) in all cases
  constexpr const_iterator find( const key_type& key ) const;            // generation checked, O(1) in all cases

                                                                         // Element access completely unique to slot_map
                                                                         // These functions could be considered "unsafe" but they offer 
                                                                         // a lookup with no branching for users confident they have a valid key
  constexpr reference find_unchecked( const key_type& key );             // unsafe, no checks, O(1) in all cases
  constexpr const_reference find_unchecked( const key_type& key ) const; // unsafe, no checks, O(1) in all cases

                                                               // Iterators
  constexpr iterator begin( );                         // O(1) in all cases
  constexpr iterator end( );                           // O(1) in all cases
  constexpr const_iterator begin( ) const;             // O(1) in all cases
  constexpr const_iterator end( ) const;               // O(1) in all cases
  constexpr const_iterator cbegin( ) const;            // O(1) in all cases
  constexpr const_iterator cend( ) const;              // O(1) in all cases
  constexpr reverse_iterator rbegin( );                // O(1) in all cases
  constexpr reverse_iterator rend( );                  // O(1) in all cases
  constexpr const_reverse_iterator rbegin( ) const;    // O(1) in all cases
  constexpr const_reverse_iterator rend( ) const;      // O(1) in all cases
  constexpr const_reverse_iterator crbegin( ) const;   // O(1) in all cases
  constexpr const_reverse_iterator crend( ) const;     // O(1) in all cases

                                             // Capacity
  constexpr bool empty( ) const;         // O(1) in all cases
  constexpr size_type size( ) const;     // O(1) in all cases
  constexpr size_type max_size( ) const; // O(1) in all cases
  constexpr size_type capacity( ) const; // O(1) in all cases
  constexpr void reserve( size_type n );  // O(N), O(1) if(empty() && n<slot_capacity() ), or if(n < capacity())

                                // Capacity unique to slot_map
  constexpr void reserve_slots( size_type n ); // O(N), O(1) if(capacity_slots() > n)
  constexpr size_type capacity_slots( ) const; // O(1) in all cases

  // Modifiers equivalent to map
  constexpr void clear( );                                                // O(N)
  constexpr key_type insert( const_reference value );                     // O(1), O(N) when allocation is required
  constexpr key_type insert( value_type&& value );                        // O(1), O(N) when allocation is required
  template<typename... Args>
  constexpr key_type emplace( Args&&... args );                           // O(1), O(N) when allocation is required
  constexpr iterator erase( iterator pos );                               // O(1) in all cases
  constexpr iterator erase( iterator first, iterator last );              // O(1) per element being erased, worst case O(N)
  constexpr iterator erase( const_iterator pos );                         // O(1) in all cases
  constexpr iterator erase( const_iterator first, const_iterator last );  // O(1) per element being erased, worst case O(N)
  constexpr size_type erase( const key_type& key );                       // O(1) in all cases. Will contain an iterator if an element was erased

                                                                // Modifiers unique to slot_map
  constexpr iterator insert_at( const_reference value, size_type slot );  // O(1) in all cases. Attempts to insert at the specified slot.
  constexpr iterator insert_at( value_type&& value, size_type slot );     // O(1) in all cases. Attempts to insert at the specified slot.
                                                                              // This should be okay given the precedent for it using an iterator set by std::set::emplace_hint
  template<typename... Args>
  constexpr iterator emplace_at( size_type slot, Args&&... args );       // O(1) in all cases. Attempts to emplace at the specified slot.

private:
  Container<key_type> m_slots;
  Container<Value> m_data;
  // this uses memory to offer O(1) for erase() overloads other than erase(const key_type&)
  Container<key_size_type> m_erase_helper;
  key_size_type m_free_head;
  key_size_type m_free_tail;

  constexpr size_type pop_head( );
  constexpr void push_tail( size_type );
  constexpr key_type finish_inserting_last_element( );
  constexpr bool key_valid( const key_type &key );
  constexpr void grow( );
  constexpr void grow_slots( );
  constexpr void pre_insert( );

  static constexpr size_type growth_rate{ 2 };
  static constexpr size_type initial_alloc_size{ static_cast<size_type>(20) };
};

#pragma region constructors_impl
template<typename Value, typename Token, template<typename...>typename Container>
constexpr slot_map<Value, Token, Container>::slot_map( )
  : m_free_head(0)
  , m_free_tail(0)
{ }
#pragma endregion

#pragma region iterators_impl

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::begin( ) {
  return m_data.begin( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::end( ) {
  return m_data.end( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_iterator slot_map<Value, Token, Container>::begin( ) const {
  return m_data.begin( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_iterator slot_map<Value, Token, Container>::end( ) const {
  return m_data.end( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_iterator slot_map<Value, Token, Container>::cbegin( ) const {
  return m_data.cbegin( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_iterator slot_map<Value, Token, Container>::cend( ) const {
  return m_data.cend( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::reverse_iterator slot_map<Value, Token, Container>::rbegin( ) {
  return m_data.rbegin( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::reverse_iterator slot_map<Value, Token, Container>::rend( ) {
  return m_data.rend( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_reverse_iterator slot_map<Value, Token, Container>::rbegin( ) const {
  return m_data.rbegin( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_reverse_iterator slot_map<Value, Token, Container>::rend( ) const {
  return m_data.rend( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_reverse_iterator slot_map<Value, Token, Container>::crbegin( ) const {
  return m_data.rbegin( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_reverse_iterator slot_map<Value, Token, Container>::crend( ) const {
  return m_data.rend( );
}
#pragma endregion

#pragma region accessors_impl


template<typename Value, typename Token, template<typename...> typename Container>
constexpr const typename slot_map<Value, Token, Container>::container_type& slot_map<Value, Token, Container>::data( ) const {
  return m_data;
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::pointer slot_map<Value, Token, Container>::at( const key_type &key ) {
  const auto &index{ std::get<0>( key ) };
  const auto &generation{ std::get<0>( key ) };
  if( index >= m_slots.size( ) || generation != *( m_slots.begin( ) + index ) )
    return nullptr;
  else
    return &find_unchecked( key );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_pointer slot_map<Value, Token, Container>::at( const key_type &key ) const {
  const auto &index{ std::get<0>( key ) };
  const auto &generation{ std::get<0>( key ) };
  if( index >= m_slots.size( ) || generation != *( m_slots.begin( ) + index ) )
    return nullptr;
  else
    return &find_unchecked( key );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_pointer slot_map<Value, Token, Container>::operator[]( const key_type &key ) const {
  const auto &index{ std::get<0>( key ) };
  const auto &generation{ std::get<1>( key ) };
  if( index >= m_slots.size( ) || generation != std::get<1>( *( m_slots.begin( ) + index ) ) )
    return nullptr;
  const auto at_index{ std::get<0>( m_slots[ index ] ) };
  return &*( m_data.cbegin( ) + at_index );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::pointer slot_map<Value, Token, Container>::operator[]( const key_type &key ) {
  const auto &index{ std::get<0>( key ) };
  const auto &generation{ std::get<0>( key ) };
  if( generation != *( m_slots.begin( ) + index ) )
    return nullptr;
  else
    return &find_unchecked( key );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::find( const key_type &key ) {
  const auto &index{ std::get<0>( key ) };
  const auto &generation{ std::get<0>( key ) };
  if( generation != *( m_slots.begin( ) + index ) )
    return end();
  const auto &at_index{ std::get<0>( m_slots[ index ] ) }
  return m_data.begin( ) + at_index;
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_iterator slot_map<Value, Token, Container>::find( const key_type& key ) const {
  const auto &index{ std::get<0>( key ) };
  const auto &generation{ std::get<0>( key ) };
  if( generation != *( m_slots.begin( ) + index ) )
    return end( );
  const auto &at_index{ std::get<0>( m_slots[ index ] ) }
  return m_data.cbegin( ) + at_index;
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::reference slot_map<Value, Token, Container>::find_unchecked( const key_type& key ) {
  const auto &index{ m_slots[ std::get<0>( key ) ] };
  return *(m_data.begin( ) + index);
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::const_reference slot_map<Value, Token, Container>::find_unchecked( const key_type& key ) const {
  const auto &index{ m_slots[ std::get<0>( key ) ] };
  return *( m_data.cbegin( ) + index );
}

#pragma endregion

#pragma region size_impl

template<typename Value, typename Token, template<typename...> typename Container>
constexpr bool slot_map<Value, Token, Container>::empty( ) const {
  return m_data.empty( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::size_type slot_map<Value, Token, Container>::size( ) const {
  return m_data.size( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::size_type slot_map<Value, Token, Container>::max_size( ) const {
  return m_data.max_size( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::size_type slot_map<Value, Token, Container>::capacity( ) const {
  return m_data.capacity( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr void slot_map<Value, Token, Container>::reserve( size_type n ) {
  if( n > capacity( ) ) {
    m_data.reserve( n );
    m_slots.reserve( n );
    m_erase_helper.reserve( n );
  }
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr void slot_map<Value, Token, Container>::reserve_slots( size_type n ) {
  if( n > capacity_slots( ) ) {
    m_slots.reserve( n );
  }
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::size_type slot_map<Value, Token, Container>::capacity_slots( ) const {
  return m_slots.capacity( );
}

#pragma endregion

#pragma region modifiers_impl

template<typename Value, typename Token, template<typename...> typename Container>
constexpr void slot_map<Value, Token, Container>::clear( ) {
  while( !m_data.empty( ) ) {
    erase( begin( ), end( ) );
  }
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::finish_inserting_last_element( ) {
  size_type key_index{ pop_head( ) };
  key_type& key{ m_slots[ key_index ] };
  auto& index = std::get<0>( key );
  auto& generation = std::get<1>(key);
  index = m_data.size( ) - 1;
  m_erase_helper.push_back( key_index );
  return { std::decay_t<decltype( index )>( key_index ), generation };
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::insert( const_reference value ) {
  pre_insert( );
  m_data.push_back( value );
  return finish_inserting_last_element( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::insert( value_type&& value ) {
  pre_insert( );
  m_data.push_back( std::move( value ) );
  return finish_inserting_last_element( );
}

template<typename Value, typename Token, template<typename...> typename Container>
template<typename... Args>
constexpr typename slot_map<Value, Token, Container>::key_type slot_map<Value, Token, Container>::emplace( Args&&... args ) {
  pre_insert( );
  m_data.emplace_back( std::forward<Args>( args )... );
  return finish_inserting_last_element( );
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::erase( iterator pos ) {
  static_assert( false, "erase() needs to be implemented." );
  if( pos == end( ) ) return pos;
  else if( pos == m_data.end( ) - 1 ) {
    m_data.pop_back( );
    push_tail( m_erase_helper.back( ) );
    m_erase_helper.pop_back( );
    return end( );
  }
  else { // erase valid element that isn't the last element
    const auto dist{ std::distance( begin( ), pos ) };
    push_tail( m_erase_helper.begin() + dist );
    *pos = std::move( m_data.back( ) );
    m_data.pop_back( );
    *(m_erase_helper.begin() + dist) = m_erase_helper.back( );
    m_erase_helper.pop_back( );
    return pos;
  }
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::erase( iterator first, iterator last ) {
  static_assert( false, "erase() needs to be implemented." );
  const auto range_size{ std::distance( first, last ) };
  const auto dist_from_end{ std::distance( last, end( ) ) };
  if( last == end( ) ) {
    for( auto it{ last - 1 }; it >= first; --it ) {
      erase( it );
    }
    return first;
  }
  else if( range_size < dist_from_end ) { // @@TODO: optimize this
    iterator separator{ first + dist_from_end };
    for( auto it{ first }; first != separator; ++it ) {
      erase( it );
    }
    assert( last == end( ) );
    for( auto it{ last - 1 }; it >= first; --it ) {
      erase( it );
    }
    // erase the first dist_from_end - range_size elements, 
    // then everything remaining to be erased reaches the end and they can be erased in reverse order
    return first;
  }
  else { // length of range does not reach the end
    for( auto it{ first }; it != last; ++it ) {
      erase( it );
    }
    return first;
  }
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::erase( const_iterator pos ) {
  const auto dist{ std::distance( cbegin( ), pos ) };
  return erase( begin( ) + dist );
}


template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::erase( const_iterator first, const_iterator last ) {
  const auto fdist{ std::distance( cbegin( ), first ) };
  const auto ldist{ std::distance( cbegin( ), last ) };
  return erase(begin() + fdist, begin()+ldist);
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::size_type slot_map<Value, Token, Container>::erase( const key_type &key ) {
  if( key_valid( key ) ) {
    erase( begin( ) + std::get<0>( m_slots[ std::get<0>( key ) ] ) );
    return 1;
  }
  return 0;
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::insert_at( const_reference value, size_type slot ) {
  // How do I figure out if a slot is in use already without walking the free list to find it?
  // bit array maybe? it's technically O(1) but consumes N/8 memory.
  return { };
}

template<typename Value, typename Token, template<typename...> typename Container>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::insert_at( value_type &&value, size_type slot ) {
  // How do I figure out if a slot is in use already without walking the free list to find it?
  // bit array maybe? it's technically O(1) but consumes N/8 memory.
  return {};
}

template<typename Value, typename Token, template<typename...> typename Container>
template<typename... Args>
constexpr typename slot_map<Value, Token, Container>::iterator slot_map<Value, Token, Container>::emplace_at( size_type slot, Args&&... args ) {
  // How can the state of a slot, be it free list or in use, be detected in constant time without using memory?
  // Bit vector could be used to make this O(1), but would require insert() and erase() to touch more memory and 
  // require an additional N/8 bytes + sizeof(vector<bool>).
  return {};
}

#pragma endregion

#pragma region helpers_impl
template<typename Value, typename Token, template<typename...>typename Container>
constexpr typename slot_map<Value, Token, Container>::size_type slot_map<Value, Token, Container>::pop_head( ) {
  if( size( ) == capacity( ) ) grow( );
  key_size_type next_free_head{ std::get<0>( m_slots[ m_free_head ] ) };
  if( next_free_head == m_free_head ) { // last free slot 
    return static_cast< size_type >( m_free_head );
  }
  else {
    key_size_type result{ m_free_head };
    m_free_head = next_free_head;
    return static_cast<size_type>( m_free_head );
  }
}

template<typename Value, typename Token, template<typename...>typename Container>
constexpr void slot_map<Value, Token, Container>::push_tail( size_type key_index ) {
  std::get<0>( m_free_tail ) = key_index;
  m_free_tail = key_index;
  ++std::get<1>( m_slots[ m_free_tail ] );
}

template<typename Value, typename Token, template<typename...>typename Container>
constexpr bool slot_map<Value, Token, Container>::key_valid( const key_type &key ) {
  return std::get<1>( key ) == m_slots[ std::get<0>( key ) ];
 
}

template<typename Value, typename Token, template<typename...>typename Container>
constexpr void slot_map<Value, Token, Container>::grow( ) {
  auto new_capacity{ capacity() * growth_rate };
  if( capacity( ) == 0 ) new_capacity = initial_alloc_size;
  if( new_capacity < capacity() ) { // overflow case
    new_capacity = std::numeric_limits<size_type>::max( );
  }
  grow_slots( );
  m_data.reserve( new_capacity );
  m_erase_helper.reserve( new_capacity );
}

template<typename Value, typename Token, template<typename...>typename Container>
constexpr void slot_map<Value, Token, Container>::grow_slots( ) {
  // @@TODO: make this function correctly interact with free list
  // @@TODO: set up new slots to chain in the free list
  // @@TODO: insert new slots at the front of the free list
  size_type initial_size{ m_slots.capacity() };
  if( initial_size ) {
    m_slots.reserve( initial_size * growth_rate );
  }
  else {
    m_slots.reserve( initial_alloc_size );
  }
  size_type capacity{ m_slots.capacity( ) };
  for( size_type i{ initial_size }; i < capacity; ++i ) {
    m_slots.emplace( m_slots.end( ), i + 1, 0 );
  }
  std::get<0>( m_slots.back( ) ) = m_free_head;
  m_free_head = initial_size;
  if( !initial_size ) m_free_tail = m_slots.size( ) - 1;
}

template<typename Value, typename Token, template<typename...>typename Container>
constexpr void slot_map<Value, Token, Container>::pre_insert( ) {
  if( size( ) == capacity( ) ) grow( );
}
#pragma endregion

// begins


/*
Questions:

Is leaving the overflow for generation counter up to the user provided type fine? (default is do nothing)
LEWG: Yes

Should clear reset generation counters (potential ABA break) or preserve them? This would be semantically different from erase(begin(), end()). Unanswered.



Discussion: accessing the underlying container. Desirable, but can break the container if insert or erase happens

- address substitutability with map and unordered_map

- email nico josuttis for help with naming and interface feedback

- add small_vector and static_vector as desirable underlying container types, thus adapter

- need examples




*/