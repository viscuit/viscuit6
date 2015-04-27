// fuzzyrewriting.h

#ifndef FUZZYREWRITING_H
#define FUZZYREWRITING_H
#include <qobject.h>
#include <qvaluevector.h>
#include <qptrlist.h>

class Page;
class vobject;
class vrule;
class FuzzyRewriting;

class mContext {
public:
  mContext();
  vrule *rule;
  Page *page;

  QValueVector<int> maxPos;
  double maxScore;
  int click;
  int score;
  int rsize;
  void matching();
  void rewriting(FuzzyRewriting *);
};


class FuzzyRewriting {
public:

  QValueVector<mContext> mcontext;
  vobject *click;
  int nclick;
  Page *top;
  int coordMode; // 0: roll, 1: delete
  int wW, wH;
  FuzzyRewriting(Page *page, vobject *obj, int coordM = 0);

  QPtrList<Page> visited;
  void collectRule(Page *p);
};

#endif
