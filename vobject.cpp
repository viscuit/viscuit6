#include "vobject.h"
#include <qpainter.h>
#include <math.h>
#include "palette.h"
#include "imagetable.h"
#include "dragoption.h"
#include "constraint.h"
#include "vrule.h"
#include "bookbase.h"
#include <qimage.h>

int vobject::serialCount = 0;
int vobject::debug = 0;
uint vobjectvector::append(vobject *f) {
	if (isEmpty()) resize(5);
	int idx;
	idx = findRef(0);
	while (idx < 0) { // fill
		idx = size();
		resize(idx*2);
		idx = findRef(0);
	}
	insert(idx, f);
	return idx;
}

bool vobjectvector::remove(vobject *f) {
  int idx = findRef(f);
  if (idx < 0) return FALSE;
  //  f->dump();
  bool xx = this->QPtrVector<vobject>::remove(idx);
//  qDebug("remove %x -> %x (%d)", f, at(idx), findRef(f));
  return xx;
}
bool vobjectvector::removeIdx(int idx) {
  if (idx < 0) return FALSE;
  vobject *f = at(idx);
  //  f->dump();
  bool xx = this->QPtrVector<vobject>::remove(idx);
//  qDebug("removeIdx %x -> %x", f, at(idx));
  return xx;
}


bool vobjectvector::contain(vobject *f) {
  int idx = findRef(f);
  return (idx >= 0);
}

vobject *vobjectvector::searchSerial(int ser) {
  ForEachFig((*this), f, if (f->serial == ser) return f);
  return 0;
}

uint vobjectvector::compaction(bool sort) {
	uint from, to = 0;
	uint hsize = size();
	for (from = 0; from < hsize; from++) {
	  vobject *f = at(from);
		if (f) {
			if (to < from) {
				insert(to, f);
				insert(from, 0);
			}
			to++;
		}
	}
	resize(to);

	if (!sort) return 0;

	// イベント図形を最初の方にもってくる
	hsize = size();
	uint eventstart = 0; 
//	for (from = 0; from < hsize; from++) {
//		if (sort) qDebug("-rule %d %d", from, at(from));
//	}

	for (from = 0; from < hsize; from++) {
		vobject *f = at(from);
		if (eventstart < from) {
		  int j;
		  for (j=from; j>eventstart; j--) {
		    insert(j, at(j-1));
		  }
		  insert(eventstart, f);
		}
		eventstart++;
	}

	return eventstart;
//	for (from = 0; from < to; from++) {
//		if (sort) qDebug("+rule %d %d", from, at(from));
//	}
}

void vobjectvector::copyFrom(vobjectvector *v) {
  clear();
  ForEachFig((*v), f, append(f->clone()));
}

QRect vobjectvector::getContentsRect() {
  QRect r;
  for (int from = 0; from < size(); from++) {
    vobject *f = at(from);
    if (f) {
      r = r.unite(f->rect);
    }
  }
  return r;
}

void vobjectvector::vacuum(QRect r, vobjectvector &vo, QPoint delta) {
  for (int from = 0; from < size(); from++) {
    vobject *f = at(from);
    if (f && f->rect.intersects(r)) {
      f->movePos(delta);
      f->updateRect();
      vo.append(f);
      insert(from, 0);
    }
  }
  compaction();
}

void vobjectvector::clone(vobjectvector &vo) {
  vo.resize(size());
  for (int from = 0; from < size(); from++) {
    vobject *f = at(from);
    if (f) {
      vobject *cc = f->clone();
      cc->highlight = f->highlight;
      vo.insert(from, cc);
    } else {
      vo.insert(from, 0);
    }
  }
}

void vobjectvector::deleteAll() {
//	qDebug("deleteAll");
  for (int from = 0; from < size(); from++) {
    vobject *f = at(from);
    if (f) {
      f->alive = 0;
//	  qDebug("del %d", from);
	  delete f;
      insert(from, 0);
    }
  }
//  qDebug("deleteALl done");

//  ForEachFig((*this), f, f->alive = 0);
  //  ForEachFig((*this), f, delete f);
}

////////////////

vobject::vobject() {
  highlight = false;
  serial = serialCount++;
  page=0;
  posDelta = QPoint(0,0);
  alive = 3122122;
  lastAngle = -5000; // initial value ....
}

vobject::~vobject() {
  alive = 0;
  //  qDebug("object %s 0x%x (0x%x)%d %d deleted", className(), this, image.bits() ,pos.x(), pos.y());
}

bool vobject::sameKind(vobject *o) {
  return o && itbl == o->itbl && id == o->id;
}

void vobject::drawTo(QPainter &p, QRect &r, QPoint pp) {
  p.drawImage(pp.x()-image.width()/2, pp.y()-image.height()/2, image);
}

void vobject::draw(QPainter &p, const QRect &r) {
  if (rect.intersects(r)) {
    //fprintf(stderr, "draw img %d %d %s/%d\n", pos.x()-shadow.width()/2, pos.y()-shadow.height()/2, itbl->fileName.ascii(), id);
    p.drawImage(pos.x()-image.width()/2, pos.y()-image.height()/2, image);

    if (debug) {
      p.drawPoint(pos);
      p.drawRect(rect);
    }
    /*
    if (highlight && !inherits("vrule")) {
      p.drawLine(pos.x()-10,pos.y(),pos.x()+10, pos.y());
      p.drawLine(pos.x(),pos.y()-10,pos.x(), pos.y()+10);
      
      //      p.drawRect(rect);
    }
    */
  } else {
    //fprintf(stderr, "no draw img %d %d %s/%d\n", pos.x()-shadow.width()/2, pos.y()-shadow.height()/2, itbl->fileName.ascii(), void);
  }
}

void vobject::drawGray(QPainter &p, const QRect &r, int alpha) {
  if (rect.intersects(r)) {
    //fprintf(stderr, "draw img %d %d %s/%d\n",
    // pos.x()-shadow.width()/2, pos.y()-shadow.height()/2, itbl->fileName.ascii(), id);
    QImage img = image.copy();
    {
      int i,j;
      for (j=0;j<img.height();j++) {
	uint *b = (uint *)img.scanLine(j);
	for (i=0;i<img.width();i++) {
	  uint alf = (((b[i]>>24)*alpha)<<16)&0xff000000;
	  b[i] = alf | (b[i]&0xffffff);
	}
      }
    }
    p.drawImage(pos.x()-image.width()/2, pos.y()-image.height()/2, img);

    if (alpha > 128 && highlight && !inherits("vrule")) {
        /*
      QPen save = p.pen();
      QPen yellow = QPen(QColor(255,255,0));
      yellow.setWidth(3);
      p.setPen(yellow);
      p.drawLine(pos.x()-5,pos.y(),pos.x()+6, pos.y());
      p.drawLine(pos.x(),pos.y()-5,pos.x(), pos.y()+6);
      p.setPen(save);
      p.drawLine(pos.x()-4,pos.y(),pos.x()+4, pos.y());
      p.drawLine(pos.x(),pos.y()-4,pos.x(), pos.y()+4);
    */
        QImage im = gBookBase->imgClick;
        p.drawImage(pos.x()-im.width()/2, pos.y()-im.height()/2, im);
    }

  } else {
    //fprintf(stderr, "no draw img %d %d %s/%d\n", pos.x()-shadow.width()/2, pos.y()-shadow.height()/2, itbl->fileName.ascii(), id);
  }
}


//
bool vobject::contains(QPoint p) {
  //  qDebug("rect contains? %d %d", p.x(),p.y());
  if (rect.contains(p)) {
    //    qDebug("rect contains %d %d", p.x(),p.y());
    QPoint pp = p - pos + QPoint(image.width()/2, image.height()/2);
    if (image.rect().contains(pp)) {
      unsigned int a = image.pixel(pp.x(), pp.y());
      //qDebug("rect contains alpha %x %d", a,(a>>24)>10);
      return (a>>24)>10;
    }
  }
  return FALSE;
}

void vobject::updateImage(double ang) {
	if (ang != lastAngle && itbl) {
		 PaletteObject * po = itbl->getObject(id);
		 if (po) {
			image = po->rotate(ang);
			lastAngle = ang;
		 }
	}
}

void vobject::updateRect() {
   updateImage(0);
 
  if (!image.isNull()) {    
    QPoint pp = pos-QPoint(image.width()/2, image.height()/2);
    rect = QRect(pp.x(), pp.y(), image.width(), image.height());
  }
}

QPoint vobject::downPoint;
QPoint vobject::downDelta;
void vobject::startDrag(QPoint p) {
  downPoint = p;
  downDelta = downPoint - pos;
}
void vobject::drag(QPoint p, DragOption &opt) {
  pos = p-downDelta;
  /*
  if (opt.grid) {
    int grid = opt.grid;
    pos = QPoint((pos.x()/grid)*grid,(pos.y()/grid)*grid);
  }
  */
  updateRect();
}

double rotvobject::downAngle;
double rotvobject::rotSence;
double rotvobject::xangle;

void rotvobject::startDrag(QPoint p) {

  downPoint = p;
  downDelta = downPoint - pos;
  downDelta = mat.invert().map(downDelta);
  xangle = downAngle = angle;


  QRect fr = rect;
  double aa;
  int l = fr.width();
  if (l < fr.height()) l = fr.height();
  aa = 2*((double)(downDelta.x()*downDelta.x()+downDelta.y()*downDelta.y()))/
    (l*l/4);
  double aa2 = (1-exp(-aa*aa));
  rotSence = aa2;

  qDebug("rotvobject startDrag %d %d %f", p.x(), p.y(), rotSence);
}

void rotvobject::drag(QPoint p, DragOption &opt) {
  //  qDebug("drag grid %d %d %d", opt.grid, opt.gridOffset.x(), opt.gridOffset.y());

  if (opt.shift && !opt.ctrl) { //水平か垂直
    QPoint dd = downPoint-p;
    int dx=dd.x(), dy=dd.y();
    if (dx<0) dx = -dx;
    if (dy<0) dy = -dy;
    if (dx > dy) { // 水平に移動
      p = QPoint(p.x(), downPoint.y());
    } else {
      p = QPoint(downPoint.x(), p.y());
    }
  }


  if (opt.grid) {
    int grid = opt.grid;  
    pos = p-downDelta;
    QPoint difp = pos-opt.gridOffset;
    int dx = difp.x();
    int dy = difp.y();
    if (dx < 0) {
      dx += (-dx/grid+1)*grid;
    }
    if (dy < 0) {
      dy += (-dy/grid+1)*grid;
    }
    dx = dx % grid;
    dy = dy % grid;
    pos = pos-QPoint(dx, dy);
    setAngle(0);
  } else {
    if (!opt.shift) {
      QWMatrix mmm;
      mmm.rotate(xangle);
      QPoint d = mmm.map(downDelta);
      QPoint llll = d + pos;
      QPoint zz = p - llll;
      double d2 = d.x()*d.x() + d.y()*d.y();
      double a, b, aa, th;
      if (d2 == 0) {
	      if (zz.x()*d.y() > zz.y()*d.x()) {
		      th = -3.14159265/2;
	      } else {
		      th = 3.14159265/2;
	      }
      } else {
	 a = ( zz.x()*d.x()+zz.y()*d.y())/d2;
	 b = (-zz.x()*d.y()+zz.y()*d.x())/d2;
         th = atan(b);
      }
 
      if (!opt.shift) th = th*rotSence;
      double a2 = th*180/3.14159265;
      xangle += a2;
      if (xangle > 360) xangle -= 360;
      if (xangle < 0) xangle += 360;
      setAngle(xangle); // rotate(a2);
      QPoint ss = p - mat.map(downDelta);
      if (opt.rule) {
	ss = opt.rule->nearPos(ss, angle);
	setAngle(angle);
      }
      if (!opt.ctrl) pos = ss;
    } else {
      if (opt.ctrl) {
	QPoint dd = p - pos;
	double th = constraint::angle(-dd.y(), dd.x());
	//	qDebug("%d %d  %f", dd.x(), dd.y(), th);
	th += downAngle;
	if (th <0) th+= 360;

	double d2;
	d2 = ((int)(th+15))/30*30;

	setAngle(d2);
      } else {
	pos = p-downDelta;
      }
    }
  }
  updateRect();
}

void rotvobject::updateRect() {
  updateImage(angle);

  if (!image.isNull()) {
    QPoint pp = pos-QPoint(image.width()/2, image.height()/2);
    rect = QRect(pp.x(), pp.y(), image.width(), image.height());
  }
}

double vobject::distance(vobject *o) {
  if (o) {
    return distance(o->pos);
  }
  return HUGE;
}
double vobject::distance(QPoint pt) {
  int dx = pos.x() - pt.x();
  int dy = pos.y() - pt.y();
  return sqrt((double)(dx*dx + dy*dy));
}

void rotvobject::click(Page *pg, QPoint pt) {
  qDebug("rotvobject click %d %d", this, pg);
  if (pg) pg->enqueue(this, 1);
}

void vobject::dump() {
  if (itbl) 
    qDebug("object %s:%x %s/%d %d %d", className(), this, itbl->fileName.ascii() , id, pos.x(), pos.y());
}

void rotvobject::dump() {
  qDebug("object %s:%x %s/%d %d %d %f", className(), this, itbl->fileName.ascii() , id, pos.x(), pos.y(), angle);
}

bool  vobject::isRegal(vobject *f) {
  if (f) {
    if (f->alive==3122122) return true;
    qDebug("fatal bbbb %d", f);
//    int x=0;
//    x = 100/x;
  }
  return false;
}
