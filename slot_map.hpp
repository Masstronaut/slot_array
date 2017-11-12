
#include <vector>
#include <tuple>
#include <limits> // find max value of key_size_type
#include <type_traits> // declval, remove_reference


template<typename T, typename Token = std::pair<unsigned, unsigned>, template<typename...>typename Container = std::vector>
class slot_map {
public:
  using key_size_type = std::remove_reference_t<decltype( std::get<0>( std::declval<Token>( ) ) )>;
  using key_generation_type = std::remove_reference_t<decltype( std::get<1>( std::declval<Token>( ) ) )>;
  using mapped_type = typename Container<T>::value_type;
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
  static_assert( sizeof( key_size_type ) <= sizeof( typename Container<T>::size_type )
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

  // Element access is O(1) in all cases.
  // at() variations have all the checks and throw if a check fails
  constexpr reference at( const key_type& key );                         // bounds+generation checked, O(1) in all cases
  constexpr const_reference at( const key_type& key ) const;             // bounds+generation checked, O(1) in all cases
  // operator[] checks generation counter and has undefined behavior for a key which fails the check.
  constexpr reference operator[]( const key_type& key );                 // generation checked, O(1) in all cases
  constexpr const_reference operator[]( const key_type& key ) const;     // generation checked, O(1) in all cases
  // returns an iterator to the element if it is found, returns an iterator to 1 past the last element otherwise.
  constexpr iterator find( const key_type& key );                        // generation checked, O(1) in all cases
  constexpr const_iterator find( const key_type& key ) const;            // generation checked, O(1) in all cases

  // These functions could be considered "unsafe" but they offer 
  // a lookup with no branching for users confident they have a valid key
  constexpr reference find_unchecked( const key_type& key );             // unsafe, no checks, O(1) in all cases
  constexpr const_reference find_unchecked( const key_type& key ) const; // unsafe, no checks, O(1) in all cases

  // Allow users
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

private:
  Container<key_type> m_slots;
  Container<T> m_data;
  // this uses memory to offer O(1) for erase()
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
  constexpr key_size_type& key_index( key_type &key );
  constexpr const key_size_type& key_index( const key_type &key ) const;
  constexpr key_generation_type& key_generation( key_type &key );
  constexpr const key_generation_type& key_generation( const key_type &key ) const;
  constexpr key_size_type& element_index( key_type &key );
  constexpr const key_size_type& element_index( const key_type &key ) const;

  static constexpr size_type growth_rate{ 2 };
  static constexpr size_type initial_alloc_size{ static_cast<size_type>(20) };
};

#pragma region constructors_impl
template<typename T, typename Token, template<typename...>typename Container>
constexpr slot_map<T, Token, Container>::slot_map( )
  : m_free_head(0)
  , m_free_tail(0)
{ }
#pragma endregion

#pragma region iterators_impl

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::begin( ) {
  return std::begin( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::end( ) {
  return std::end( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_iterator slot_map<T, Token, Container>::begin( ) const {
  return std::begin( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_iterator slot_map<T, Token, Container>::end( ) const {
  return std::end( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_iterator slot_map<T, Token, Container>::cbegin( ) const {
  return std::cbegin( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_iterator slot_map<T, Token, Container>::cend( ) const {
  return std::cend( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::reverse_iterator slot_map<T, Token, Container>::rbegin( ) {
  return std::rbegin( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::reverse_iterator slot_map<T, Token, Container>::rend( ) {
  return std::rend( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reverse_iterator slot_map<T, Token, Container>::rbegin( ) const {
  return std::rbegin( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reverse_iterator slot_map<T, Token, Container>::rend( ) const {
  return std::rend( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reverse_iterator slot_map<T, Token, Container>::crbegin( ) const {
  return std::rbegin( m_data );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reverse_iterator slot_map<T, Token, Container>::crend( ) const {
  return std::rend( m_data );
}
#pragma endregion

#pragma region accessors_impl


template<typename T, typename Token, template<typename...> typename Container>
constexpr const typename slot_map<T, Token, Container>::container_type& slot_map<T, Token, Container>::data( ) const {
  return m_data;
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::reference slot_map<T, Token, Container>::at( const key_type &key ) {
  const auto &index{ this->key_index( key ) };
  const auto &generation{ this->key_generation( key ) };
  if( index >= m_slots.size( ) || generation != key_generation( *( m_slots.begin( ) + index ) ) )
    throw std::range_error( "Invalid key used with slot_map::at." );
  else
    return find_unchecked( key );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reference slot_map<T, Token, Container>::at( const key_type &key ) const {
  const auto &index{ key_index( key ) };
  const auto &generation{ key_generation( key ) };
  if( index >= m_slots.size( ) || generation != this->key_generation( *( std::begin( m_slots ) + index ) ) )
    throw std::range_error("Invalid key used with slot_map::at." );
  else
    return this->find_unchecked( key );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reference slot_map<T, Token, Container>::operator[]( const key_type &key ) const {
  const auto &index{ this->key_index( key ) };
  const auto &generation{ this->key_generation( key ) };
  if( index >= m_slots.size( ) || generation != this->key_generation( *( std::begin( m_slots ) + index ) ) )
    throw std::range_error( "Invalid key used with slot_map::operator[]." );
  return *( std::cbegin( m_data ) + this->element_index( key ) );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::reference slot_map<T, Token, Container>::operator[]( const key_type &key ) {
  const auto &index{ this->key_index( key ) };
  const auto &generation{ this->key_generation( key ) };
  if( generation != key_generation( *( m_slots.begin( ) + index ) ) )
    throw std::range_error("Invalid key used with slot_map::operator[].");
  else
    return this->find_unchecked( key );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::find( const key_type &key ) {
  const auto &index{ this->key_index( key ) };
  const auto &generation{ this->key_generation( key ) };
  if( generation != *( m_slots.begin( ) + index ) )
    return this->end();
  return std::begin( m_data ) + element_index( key );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_iterator slot_map<T, Token, Container>::find( const key_type& key ) const {
  const auto &index{ this->key_index( key ) };
  const auto &generation{ this->key_generation( key ) };
  if( generation != *( m_slots.begin( ) + index ) )
    return this->end( );
  return this->cbegin( ) + this->element_index( key );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::reference slot_map<T, Token, Container>::find_unchecked( const key_type& key ) {
  return *( std::begin( m_data ) + this->element_index( key ) );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::const_reference slot_map<T, Token, Container>::find_unchecked( const key_type& key ) const {
  return *(std::cbegin( m_data ) + this->element_index( key ));
}

#pragma endregion

#pragma region size_impl

template<typename T, typename Token, template<typename...> typename Container>
constexpr bool slot_map<T, Token, Container>::empty( ) const {
  return m_data.empty( );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::size_type slot_map<T, Token, Container>::size( ) const {
  return static_cast<size_type>(m_data.size( ));
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::size_type slot_map<T, Token, Container>::max_size( ) const {
  return static_cast<size_type>( m_data.max_size( ) );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::size_type slot_map<T, Token, Container>::capacity( ) const {
  return static_cast<size_type>( m_data.capacity( ) );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr void slot_map<T, Token, Container>::reserve( size_type n ) {
  if( n > this->capacity( ) ) {
    m_data.reserve( n );
    m_slots.reserve( n );
    this->grow_slots( );
    m_erase_helper.reserve( n );
  }
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr void slot_map<T, Token, Container>::reserve_slots( size_type n ) {
  if( n > this->capacity_slots( ) ) {
    m_slots.reserve( n );
    this->grow_slots( );
  }
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::size_type slot_map<T, Token, Container>::capacity_slots( ) const {
  return m_slots.capacity( );
}

#pragma endregion

#pragma region modifiers_impl

template<typename T, typename Token, template<typename...> typename Container>
constexpr void slot_map<T, Token, Container>::clear( ) {
  m_data.clear( );
  m_slots.clear( );
  m_erase_helper.clear( );
  this->grow_slots( );

}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::key_type slot_map<T, Token, Container>::finish_inserting_last_element( ) {
  size_type slot_index{ pop_head( ) };
  key_type& key{ *( std::begin( m_slots ) + slot_index ) };
  key_size_type &index = this->key_index( key );
  const key_generation_type& generation = this->key_generation( key );
  index = static_cast< size_type >( m_data.size( ) - 1 );
  m_erase_helper.push_back( slot_index );
  return { static_cast<key_size_type>( slot_index ), generation };
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::key_type slot_map<T, Token, Container>::insert( const_reference value ) {
  this->pre_insert( );
  m_data.push_back( value );
  return this->finish_inserting_last_element( );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::key_type slot_map<T, Token, Container>::insert( value_type&& value ) {
  this->pre_insert( );
  m_data.push_back( std::move( value ) );
  return this->finish_inserting_last_element( );
}

template<typename T, typename Token, template<typename...> typename Container>
template<typename... Args>
constexpr typename slot_map<T, Token, Container>::key_type slot_map<T, Token, Container>::emplace( Args&&... args ) {
  this->pre_insert( );
  m_data.emplace_back( std::forward<Args>( args )... );
  return this->finish_inserting_last_element( );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::erase( iterator pos ) {
  if( pos == this->end( ) ) return pos;
  else if( pos == ( std::end( m_data ) - 1 ) ) {
    m_data.pop_back( );
    this->push_tail( m_erase_helper.back( ) );
    m_erase_helper.pop_back( );
    return this->end( );
  }
  else { // erase valid element that isn't the last element
    const auto dist{ std::distance( this->begin( ), pos ) };
    const key_size_type slot_index{ *( m_erase_helper.begin( ) + dist ) };
    this->push_tail( slot_index );
    *pos = std::move( m_data.back( ) );
    m_data.pop_back( );
    const key_size_type update_slot{ m_erase_helper.back( ) };
    *( m_erase_helper.begin( ) + dist ) = m_erase_helper.back( );
    m_erase_helper.pop_back( );
    this->key_index( *( std::begin( m_slots ) + update_slot ) ) = dist;
    return pos;
  }
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::erase( iterator first, iterator last ) {
  static_assert( false, "erase() needs to be implemented." );
  const auto range_size{ std::distance( first, last ) };
  const auto dist_from_end{ std::distance( last, this->end( ) ) };
  if( last == this->end( ) ) {
    for( auto it{ last - 1 }; it >= first; --it ) {
      this->erase( it );
    }
    return first;
  }
  else if( range_size < dist_from_end ) { // @@TODO: optimize this
    iterator separator{ first + dist_from_end };
    for( auto it{ first }; it != separator; ++it ) {
      this->erase( it );
    }
    assert( last == end( ) );
    for( auto it{ last - 1 }; it >= first; --it ) {
      this->erase( it );
    }
    // erase the first dist_from_end - range_size elements, 
    // then everything remaining to be erased reaches the end and they can be erased in reverse order
    return first;
  }
  else { // length of range does not reach the end
    for( auto it{ first }; it != last; ++it ) {
      this->erase( it );
    }
    return first;
  }
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::erase( const_iterator pos ) {
  const auto dist{ std::distance( this->cbegin( ), pos ) };
  return this->erase( this->begin( ) + dist );
}


template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::iterator slot_map<T, Token, Container>::erase( const_iterator first, const_iterator last ) {
  const auto fdist{ std::distance( this->cbegin( ), first ) };
  const auto ldist{ std::distance( this->cbegin( ), last ) };
  return this->erase( this->begin( ) + fdist, this->begin( ) + ldist );
}

template<typename T, typename Token, template<typename...> typename Container>
constexpr typename slot_map<T, Token, Container>::size_type slot_map<T, Token, Container>::erase( const key_type &key ) {
  if( this->key_valid( key ) ) {
    this->erase( this->begin( ) + this->element_index( key ) );
    return 1;
  }
  return 0;
}

#pragma endregion

#pragma region helpers_impl
template<typename T, typename Token, template<typename...>typename Container>
constexpr typename slot_map<T, Token, Container>::size_type slot_map<T, Token, Container>::pop_head( ) {
  if( this->size( ) == this->capacity( ) )this->grow( );
  key_size_type next_free_head{ this->key_index( *( std::begin( m_slots ) + m_free_head ) ) };
  if( next_free_head == m_free_head ) { // last free slot 
    assert( m_data.size( ) == m_data.capacity( ) );
    return static_cast< size_type >( m_free_head );
  }
  else {
    key_size_type result{ m_free_head };
    m_free_head = next_free_head;
    return static_cast<size_type>( result );
  }
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr void slot_map<T, Token, Container>::push_tail( size_type tail_index ) {
  this->key_index( *( std::begin( m_slots ) + m_free_tail ) ) = tail_index;
  this->key_index( *( std::begin( m_slots ) + tail_index ) ) = tail_index;
  m_free_tail = tail_index;
  ++this->key_generation( *( std::begin( m_slots ) + tail_index ) );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr bool slot_map<T, Token, Container>::key_valid( const key_type &key ) {
  return this->key_generation( key ) == this->key_generation( *( std::begin( m_slots ) + key_index( key ) ) );
 
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr void slot_map<T, Token, Container>::grow( ) {
  auto new_capacity{ capacity( ) * growth_rate };
  if( capacity( ) == 0 ) new_capacity = initial_alloc_size;
  if( new_capacity < capacity( ) ) { // overflow case
    new_capacity = std::numeric_limits<size_type>::max( );
  }
  grow_slots( );
  m_data.reserve( new_capacity );
  m_erase_helper.reserve( new_capacity );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr void slot_map<T, Token, Container>::grow_slots( ) {
  // @@TODO: make this function correctly interact with free list // possibly done - now adds the tail correctly
  // @@TODO: insert new slots at the front of the free list
  size_type initial_size{ static_cast<size_type>(m_slots.capacity()) };
  if( initial_size ) {
    m_slots.reserve( initial_size * growth_rate );
  }
  else {
    m_slots.reserve( initial_alloc_size );
  }
  size_type capacity{ static_cast< size_type >( m_slots.capacity( ) ) };
  for( size_type i{ initial_size }; i < capacity; ++i ) {
    m_slots.emplace( std::end( m_slots ), i + 1, 0 );
  }
  this->key_index( m_slots.back( ) ) = static_cast< key_size_type >( m_slots.size( ) - 1 );
  m_free_head = initial_size;
  m_free_tail = this->key_index( m_slots.back( ) );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr void slot_map<T, Token, Container>::pre_insert( ) {
  if( this->size( ) == this->capacity( ) ) this->grow( );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr typename slot_map<T, Token, Container>::key_size_type& slot_map<T, Token, Container>::key_index( key_type &key ) {
  return std::get<0>( key );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr const typename slot_map<T, Token, Container>::key_size_type & slot_map<T, Token, Container>::key_index( const key_type &key ) const {
  return std::get<0>( key );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr typename slot_map<T, Token, Container>::key_generation_type & slot_map<T, Token, Container>::key_generation( key_type &key ) {
  return std::get<1>( key );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr const typename slot_map<T, Token, Container>::key_generation_type & slot_map<T, Token, Container>::key_generation( const key_type &key ) const {
  return std::get<1>( key );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr typename slot_map<T, Token, Container>::key_size_type & slot_map<T, Token, Container>::element_index( key_type &key ) {
  return this->key_index( std::begin( m_slot ) + this->key_index( key ) );
}

template<typename T, typename Token, template<typename...>typename Container>
constexpr const typename slot_map<T, Token, Container>::key_size_type & slot_map<T, Token, Container>::element_index( const key_type &key ) const {
  return this->key_index( *(std::begin( m_slots ) + this->key_index( key ) ) );
}



#pragma endregion

// begins


/*
Questions:

Is leaving the overflow for generation counter up to the user provided type fine? (default is do nothing)
LEWG: Yes

Should clear reset generation counters (potential ABA break) or preserve them? This would be semantically different from erase(begin(), end()).
SG14: Yes



Discussion: accessing the underlying container. Desirable, but can break the container if insert or erase happens

- address substitutability with map and unordered_map

- add small_vector and static_vector as desirable underlying container types, thus adapter

- need examples




*/