#ifndef GPTRVECTOR_H
#define GPTRVECTOR_H
#include <qptrvector.h>

template <class T>
class GPtrVector : public QPtrVector<T> {
public:
  uint append(T *f) {
    if (isEmpty()) resize(5);
    int idx;
    idx = findRef(0);
    while (idx < 0) {
      idx = size();
      resize(idx*2);
      idx = findRef(0);
    }
    insert(idx, f);
    return idx;
  }
  bool remove(T *f) {
    int idx = findRef(f);
    if (idx < 0) return FALSE;
    return this->QPtrVector<T>::remove(idx);
  }
  void copyFrom(GPtrVector<T> *v) {
    clear();
    for (uint i=0; i<v->size(); i++) {
      T* f = v->at(i);
      if (f) {
	append(f->clone());
      }
    }
  }
};

#endif
