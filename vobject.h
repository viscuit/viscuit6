// vobject.h
#ifndef VOBJECT_H
#define VOBJECT_H

#include <qobject.h>
#include <qptrvector.h>
#include <qwmatrix.h>
#include <qimage.h>
class QPainter;
class QRect;
class ImageTable;
class QDataStream;
class Page;
class DragOption;

class vobject : public QObject {
  Q_OBJECT
    public:

  Page *page;
  QPoint pos;
  QPoint posDelta; // 移動のとき前の場所との差 (速度?)
  QRect rect;
  ImageTable *itbl;
  int id;
  int serial;
  static int serialCount;

  bool sameKind(vobject *o);
  QImage  image;

  bool highlight;
  int alive;

  static bool isRegal(vobject *);
  vobject();
  ~vobject();
  virtual vobject *clone()=0;
  virtual void drawTo(QPainter &, QRect &, QPoint); // vrule にはうごかないよ
  virtual void draw(QPainter &, const QRect &);
  virtual void drawGray(QPainter &, const QRect &, int alpha); // alpha=0..256

  virtual bool contains(QPoint p);
  virtual void updateRect();
  static QPoint downPoint;
  static QPoint downDelta;
  virtual void startDrag(QPoint p);
  virtual void drag(QPoint p, DragOption &d);
  void setPos(QPoint p) { pos = p; updateRect(); }
  void movePos(QPoint p) { pos += p; updateRect(); }
  void set(ImageTable *it, int i) {itbl = it; id = i;}
  virtual bool dropOk(QPoint p) { return contains(p); }
  virtual bool dropOk(vobject *o) { return rect.intersects(o->rect); }
  virtual void drop(vobject *o) { delete o; }

  double distance(vobject *o);
  double distance(QPoint p);
  virtual bool overlapped(vobject *other) { return FALSE; }

  static vobject *read(QDataStream &stream);
  virtual void write(QDataStream &stream);
  virtual void rawWrite(QDataStream &stream);
  virtual void rawRead(QDataStream &stream);

  virtual void setup() { updateRect(); }

  virtual void click(Page *pg, QPoint pt) { qDebug("click vobject %d", this); }
  virtual void setGrid(int g) {}

  static int debug;

  virtual bool visibleInFixed() { return false; }

  virtual bool isAttribute(int whichAtri) { return false; }
  virtual bool isAttribute(int from, int to) { return false; }
  virtual void dump();

  double lastAngle;
  void updateImage(double angle);  
};

class rotvobject : public vobject {
  Q_OBJECT
    public:
  double angle;
  QWMatrix mat;
  static double downAngle;
  static double rotSence;
  static double xangle;

  rotvobject() : vobject() { angle = 0; mat.reset(); }
  void rotate(double ang) {
    angle += ang;
    if (angle > 360) angle -= 360;
    if (angle > 360) angle -= 360;
    if (angle < 0) angle += 360;
    if (angle < 0) angle += 360;
    mat.rotate(ang);
    updateRect();
  }
  void setAngle(double ang) {
    angle = 0; mat.reset();
    rotate(ang);
  }

  virtual void setup() { setAngle(angle); }

  virtual void updateRect();

  virtual void startDrag(QPoint p);
  virtual void drag(QPoint p, DragOption &d);

  virtual void rawWrite(QDataStream &stream);
  virtual void rawRead(QDataStream &stream);
  virtual void click(Page *pg, QPoint pt);
  virtual bool visibleInFixed() { return true; }
  virtual void dump();
};

class vobjectvector : public QPtrVector<vobject> {
public:
  uint append(vobject *f);
  bool remove(vobject *f);
  bool removeIdx(int idx);
  bool contain(vobject *f);
  uint compaction(bool sort=FALSE);
  void copyFrom(vobjectvector *v);
  QRect getContentsRect();

  void write(QDataStream &stream);
  void read(QDataStream &stream);
  vobject *searchSerial(int ser);

  void vacuum(QRect r, vobjectvector &vo, QPoint ); // この矩形と重なっているオブジェクトをとりだす
  
  void deleteAll();
  void clone(vobjectvector &result);
};

#define ForEachFig(_vec, _f, _body) {	for (uint __i=0;__i<_vec.size();__i++) { vobject *_f = _vec.at(__i);if (vobject::isRegal(_f)) { _body; }}}

#define ForEachFigRev(_vec, _f, _body) {for (int __i=_vec.size()-1;__i >= 0;__i--) { vobject *_f = _vec.at(__i);if (vobject::isRegal(_f)) { _body; }}}

#endif
