// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#ifndef data_list_h
#define data_list_h

#include "array.h"
#include "common/common.h"
#include "memory/ptr.h"

namespace codeswitch {

/**
 * A variable length, contiguous sequence of elements of the same type.
 *
 * List uses Array as a backing store. List keeps track of the size of the
 * Array (cap) and the number of elements in use (length). If the client
 * attempts to append more than cap elements, the existing elements are
 * moved to a new, larger array first. As a result, append takes amortized
 * O(1) time.
 */
template <class T>
class List {
 public:
  List();
  List(const List& list);
  List(List&& list);
  List& operator=(const List& list);
  List& operator=(List&& list);
  static List* make(length_t cap);

  const T& at(length_t i) const { return (*this)[i]; }
  T& at(length_t i) { return (*this)[i]; }
  const T& operator[](length_t i) const;
  T& operator[](length_t i);
  length_t length() const { return length_; }
  length_t cap() const { return cap_; }
  void append(const T& elem);
  void append(T* elems, length_t n);

  template <class IT>
  class iterator : public std::iterator<std::forward_iterator_tag, T> {
   public:
    explicit iterator(IT* p) : p_(p) {}
    iterator operator++(int) { return p_++; }
    iterator& operator++() {
      ++p_;
      return *this;
    }
    IT& operator*() const { return *p_; }
    IT* operator->() const { return p_; }
    bool operator==(const iterator& that) { return p_ == that.p_; }
    bool operator!=(const iterator& that) { return p_ != that.p_; }

   private:
    IT* p_;
  };
  iterator<T> begin() { return iterator(&data_->at(0)); }
  iterator<T> end() { return iterator(&data_->at(length_)); }
  iterator<const T> begin() const { return iterator(&data_->at(0)); }
  iterator<const T> end() const { return iterator(&data_->at(length_)); }

 private:
  void resize(length_t newLength);

  Ptr<Array<T>> data_;
  length_t length_;
  length_t cap_;
};

template <class T>
List<T>::List() : length_(0), cap_(0) {}

template <class T>
List<T>::List(const List<T>& list) :
    data_(const_cast<Array<T>*>(list.data_.get())), length_(list.length_), cap_(list.cap_) {}

template <class T>
List<T>::List(List<T>&& list) : data_(list.data_.get()), length_(list.length_), cap_(list.cap_) {
  list.data_.set(nullptr);
  list.length_ = 0;
  list.cap_ = 0;
}

template <class T>
List<T>& List<T>::operator=(const List<T>& list) {
  data_.set(list.data_.get());
  length_ = list.length_;
  cap_ = list.cap_;
  return *this;
}

template <class T>
List<T>& List<T>::operator=(List<T>&& list) {
  data_.set(list.data_.get());
  length_ = list.length_;
  cap_ = list.cap_;
  list.data_.set(nullptr);
  list.length_ = 0;
  list.cap_ = 0;
  return *this;
}

template <class T>
List<T>* List<T>::make(length_t cap) {
  auto list = new (heap.allocate(sizeof(List<T>))) List<T>();
  list->data_.set(Array<T>::make(cap));
  list->cap_ = cap;
  return list;
}

template <class T>
const T& List<T>::operator[](length_t i) const {
  if (i >= length_) {
    throw BoundsCheckError();
  }
  return data_->at(i);
}

template <class T>
T& List<T>::operator[](length_t i) {
  if (i >= length_) {
    throw BoundsCheckError();
  }
  return data_->at(i);
}

template <class T>
void List<T>::append(const T& elem) {
  auto i = length_;
  resize(length_ + 1);
  data_->at(i) = elem;
}

template <class T>
void List<T>::append(T* elems, length_t n) {
  resize(length_ + n);
  for (length_t i = 0; i < n; i++) {
    data_->at(length_ - n + i) = elems[i];
  }
}

template <class T>
void List<T>::resize(length_t newLength) {
  if (newLength <= length_) {
    for (length_t i = length_; i < newLength; i++) {
      data_->at(i) = T();
    }
    length_ = newLength;
    return;
  }

  auto newCap = cap_;
  while (newCap < newLength) {
    newCap *= 2;
    if (newCap < 8) {
      newCap = 8;
    }
  }
  if (newCap > cap_) {
    auto newData = Array<T>::make(newCap);
    for (length_t i = 0; i < length_; i++) {
      newData->at(i) = data_->at(i);
    }
    data_.set(newData);
  }
  length_ = newLength;
  cap_ = newCap;
}

}  // namespace codeswitch

#endif
