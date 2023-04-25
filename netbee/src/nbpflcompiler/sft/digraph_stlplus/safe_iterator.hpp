#ifndef STLPLUS_SAFE_ITERATOR
#define STLPLUS_SAFE_ITERATOR
////////////////////////////////////////////////////////////////////////////////

//   Author:    Andy Rushton
//   Copyright: (c) Andy Rushton, 2007
//   License:   BSD License, see license.html

//   The STLplus safe_iterator superclasses. This implements the STLplus safe
//   iterator principles. Data structures can then be built using subclasses
//   of safe_iterator for their iterator objects and they will inherit the
//   safe iterator behaviour.

//   The data structure must contain a master iterator for each node in the
//   structure. When an iterator is returned to the user, it must be created
//   by the master iterator. When a node is removed from the data structure,
//   its master iterator is destroyed. This sets all iterators pointing to the
//   master iterator to end iterators.

////////////////////////////////////////////////////////////////////////////////
#include "containers_fixes.hpp"
#include "exceptions.hpp"

namespace stlplus
{

  ////////////////////////////////////////////////////////////////////////////////
  // internals

  template<typename O, typename N>
  class safe_iterator_body;

  template<typename O, typename N>
  class safe_iterator;

  ////////////////////////////////////////////////////////////////////////////////
  // Master Iterator
  // Create one of these in each node in the data structure
  // Generate iterators by obtaining a safe-iterator object from the master iterator
  ////////////////////////////////////////////////////////////////////////////////

  template<typename O, typename N>
  class master_iterator
  {
  public:

    // construct a valid master iterator connected to the node
    master_iterator(const O* owner, N* node) throw();

    // destructor - disconnects all iterators from the node
    ~master_iterator(void) throw();

    // dereference
    N* node(void) const throw();
    const O* owner(void) const throw();

    // when you move a node from one owner to another, call this on the node's master iterator
    // this effectively moves all other iterators to the node so that they are owned by the new owner too
    void change_owner(const O* owner) throw();

    friend class safe_iterator<O,N>;
  private:
    master_iterator(const master_iterator&) throw();
    master_iterator& operator=(const master_iterator&) throw();
    safe_iterator_body<O,N>* m_body;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Safe Iterator
  ////////////////////////////////////////////////////////////////////////////////

  template<typename O, typename N>
  class safe_iterator
  {
  public:

    // construct a null iterator
    safe_iterator(void) throw();

    // construct a valid iterator by aliasing from the owner node's master iterator
    safe_iterator(const master_iterator<O,N>&) throw();

    // copy constructor does aliasing
    safe_iterator(const safe_iterator<O,N>&) throw();

    // alias an iterator by assignment
    safe_iterator<O,N>& operator=(const safe_iterator<O,N>&) throw();

    // destructor
    ~safe_iterator(void) throw();

    // reassignment to another node used in increment/decrement operation
    void set(const master_iterator<O,N>&) throw();

    // dereference
    N* node(void) const throw();
    const O* owner(void) const throw();

    // change to a null iterator - i.e. one that does not belong to any object
    // this does not affect any other iterators pointing to the same node
    void set_null(void) throw();

    ////////////////////////////////////////////////////////////////////////////////
    // operations for clients that do not have a master end iterator
    // alternatively, have a master end iterator as part of the container
    // and call constructor(master_end) or set(master_end)

    // construct an end iterator
    safe_iterator(const O* owner) throw();

    // change to an end iterator - e.g. as a result of incrementing off the end
    void set_end(void) throw();

    ////////////////////////////////////////////////////////////////////////////////
    // tests

    // comparison
    bool equal(const safe_iterator<O,N>& right) const throw();
    int compare(const safe_iterator<O,N>& right) const throw();

    // a null iterator is one that has not been initialised with a value yet
    // i.e. you just declared it but didn't assign to it
    bool null(void) const throw();

    // an end iterator is one that points to the end element of the list of nodes
    // in STL conventions this is one past the last valid element and must not be dereferenced
    bool end(void) const throw();

    // a valid iterator is one that can be dereferenced
    // i.e. non-null and non-end
    bool valid(void) const throw();

    // check the rules for a valid iterator that can be dereferenced
    // optionally also check that the iterator is owned by the owner
    void assert_valid(void) const throw(null_dereference,end_dereference);
    void assert_valid(const O* owner) const throw(wrong_object,null_dereference,end_dereference);
    // assert the rules for a non-null iterator - i.e. valid or end, values that occur in increment operations
    void assert_non_null(void) const throw(null_dereference);
    // assert that this iterator is owned by this container
    void assert_owner(const O* owner) const throw(wrong_object);

    ////////////////////////////////////////////////////////////////////////////////

    friend class master_iterator<O,N>;
  private:
    safe_iterator_body<O,N>* m_body;
  };

  ////////////////////////////////////////////////////////////////////////////////

} // end namespace stlplus

#include "safe_iterator.tpp"
#endif
