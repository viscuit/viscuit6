// vrule.h
#ifndef VRULE_H
#define VRULE_H

#include "vobject.h"
#include <qsize.h>
#include <qrect.h>
class QPainter;

class constraint;

class vrule : public vobject {
  Q_OBJECT
    public:

  vrule();
  ~vrule();
  vobject *clone();

  int grid;
  virtual void setGrid(int g) { grid = g; }

  virtual void draw(QPainter &, const QRect &);
  vobjectvector head;
  vobjectvector body;

  QSize size;

  QRect headRect;
  QRect bodyRect;
  void updateRect();
  bool dropOk(QPoint p) { return headRect.contains(p) || bodyRect.contains(p); }
  void drop(vobject *o);

  vobject *dragRule(QPoint p);
  void resize();
  void checkEvent2();
  void checkEvent();

  void catchObject(vobject *obj);
  bool dirty;

  constraint *hc, *bc, *hbc;
  vobject *withheadevent;

  vobjectvector headnoevent;
  int headnoeventcount;
  vobjectvector bodyevent;

  vobjectvector headAttribute; 
  int headAttributeCount;

  void calcConstraint();
  bool rmatch(vobject *o);
  virtual bool contains(QPoint p);

  virtual void rawWrite(QDataStream &stream);
  virtual void rawRead(QDataStream &stream);
  virtual void setup() { checkEvent(); vobject::updateRect(); resize(); updateRect();}
  virtual void drag(QPoint p, DragOption &opt);

  void click(Page *, QPoint p);

  QPoint nearPos(QPoint p, double &angle); 
};


#endif

