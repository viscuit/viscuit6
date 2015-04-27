#include "vrule.h"
#include "bookbase.h"
#include "page.h"
#include "vevent.h"
#include <qpainter.h>
#include "constraint.h"
#include "binder.h"
#include "book.h"

vrule::vrule() : vobject() {
  size = QSize(50, 50);
  hc = bc = hbc = 0;
  grid = 0;
  dirty = FALSE;
  withheadevent = 0;
}

vrule::~vrule() {
  //  qDebug("object %s %d %d %d deleted", className(), this, pos.x(), pos.y());

  ForEachFig(head, f, delete f);
  ForEachFig(body, f, delete f);
//  this->vobject::~vobject();
}

vobject *vrule::clone() {
  vrule *f = new vrule();
  f->itbl = itbl;
  f->id = id;
  f->pos = pos;
  f->updateRect();
  f->grid = grid;

  if (head.size())
    head.clone(f->head);
  if (body.size())
    body.clone(f->body);

  //  qDebug("head size %d %d", head.size(), f->head.size());

  int idx;
  if (withheadevent && (idx = head.findRef(withheadevent)) >= 0) {
    qDebug("findref %d %d", idx, f->head.size());
    f->withheadevent = f->head[idx];
  } else {
    f->withheadevent = 0;
  }
  f->checkEvent();
  f->resize();
  f->updateRect();
  return f;
}


void vrule::updateRect() {
  vobject::updateRect();
  if (page && page->inherits("Palette")) return;
  rect = rect.unite(headRect);
  rect = rect.unite(bodyRect);
}

void vrule::draw(QPainter &p, const QRect &rct) {
  QRect r = rct;

  if (!rect.intersects(r)) return;

  //  fprintf(stderr, "vrule draw rect %d %d %d %d   %d %d %d %d\n", rect.left(), rect.top(), rect.right(), rect.bottom(), r.left(), r.top(), r.right(), r.bottom());

  vobject::draw(p, r);

  if (page && page->inherits("Palette")) return;

  QRect hr = headRect;
  QRect br = bodyRect;

  QPen pen(QColor(0,0,0));
  if (highlight) {
    pen.setStyle(Qt::SolidLine);
  } else {
    pen.setStyle(Qt::DotLine);
  }
  p.setPen(pen);
  p.drawRect(hr);
  p.drawRect(br);
  pen.setStyle(Qt::SolidLine);
  p.setPen(pen);

  p.translate(hr.left(), hr.top());
  if (hr.intersects(r)) {
    r.moveBy(-hr.left(), -hr.top());
    ForEachFig(head, f, if (!f->inherits("vevent")) f->drawGray(p, r, gBookBase->confRuleTransparency));
    r.moveBy(hr.left(), hr.top());
  }
  /*
    やっぱりやめよう
  if (head.size() > 0) {
    // ヘッドにオブジェクトが入ると
    if (BookBase::defaultTool) {
      vobject *to = head[0];
      if (to) {
	vobject *event = BookBase::defaultTool->objects[1];
	// イベント図形をそのオブジェクトの中心に描くよ
	//	qDebug("rule %d %d %d %d", BookBase::defaultTool->objects[0],
	// event, to->pos.x(), to->pos.y());
	if (event) event->drawTo(p, r, to->pos);
      }
    }
  }
  */
  p.translate(br.left()-hr.left(), 0);
  if (br.intersects(r)) {
    r.moveBy(-br.left(), -br.top());
    ForEachFig(head, f, if (!f->inherits("vevent")) f->drawGray(p, r, gBookBase->confRuleHeadShadow));
    ForEachFig(body, f, if (!f->inherits("vevent")) f->drawGray(p, r, gBookBase->confRuleTransparency));
    r.moveBy(br.left(), br.top());
  }
  p.translate(-br.left(), -hr.top());

  /*
  ForEachFig(head, f, qDebug("rule head %d", f));
  ForEachFig(body, f, qDebug("rule body %d", f));
  */
}

void vrule::drop(vobject *o) {
  if (o->inherits("vrule")) return;
  if (o->pos.x() < pos.x()) {
    o->movePos(QPoint(-headRect.left(), -headRect.top()));
    head.append(o);
    checkEvent();
    resize();
    updateRect();
  } else {
    o->movePos(QPoint(-bodyRect.left(), -bodyRect.top()));
    body.append(o);
    checkEvent();
    resize();
    updateRect();
  }
}

vobject *vrule::dragRule(QPoint p) {
  QRect hr = headRect;
  QRect br = bodyRect;
  QPoint delta;
  vobjectvector *vec;
  if (hr.contains(p)) {
    delta = QPoint(hr.left(), hr.top());
    vec = &head;
  } else if (br.contains(p)) {
    delta  = QPoint(br.left(), br.top());
    vec = &body;
  } else {
    return 0;
  }
  QPoint pp = p - delta;
  ForEachFigRev((*vec), f, if (!f->inherits("vevent") && f->contains(pp)) {
    vec->remove(f); f->movePos(delta); return f;});
  return 0;
}

void vrule::drag(QPoint p, DragOption &opt) {
  vobject::drag(p, opt);
  resize();
  updateRect();
}

void vrule::resize() {
  QRect r;

  ForEachFig(head, f, r = r.unite(f->rect));
  ForEachFig(body, f, r = r.unite(f->rect));
  if (r.isEmpty()) {
    r = QRect(0, 0, 50, 50);
  }

  /*
  int left = (r.left()/grid)*grid; if (r.left() < 0) left-=grid;
  int top = (r.top()/grid)*grid;   if (r.top() < 0)  top -=grid;
  int right = r.right();
  int bottom = r.bottom();
  */

  int left = r.left();
  int top = r.top();
  int right = r.right();
  int bottom = r.bottom();


  QPoint pt = QPoint(-left, -top);
  ForEachFig(head, f, f->movePos(pt));
  ForEachFig(body, f, f->movePos(pt));


  ForEachFig(head, f,  
	     ForEachFig(body, ff, 
			if (ff->distance(f) < 5) {
			  ff->setPos(f->pos);
			}));

  size = QSize(right-left, bottom-top);
  int w;
  if (grid) w = ((image.width()+grid-1)/grid)*grid ;
  else w = image.width();

  int x1 = pos.x()-w/2-size.width();
  int x2 = pos.x()+(w-w/2); 
  int h = pos.y() - size.height()/2;

  headRect = QRect(x1, h, size.width()+1, size.height()+1);
  bodyRect = QRect(x2, h, size.width()+1, size.height()+1);

}

void vrule::checkEvent2() {
  // ここでやること
  // head にはちゃんとclickイベントがあるか
  // body にclickイベントがあったら，それは浮いていないか
  // いずれのイベント図形も必ずvectorの最後に置かれること

  int eventcnt = 0;


  vevent *clickevent = 0;
  withheadevent = 0;

  ForEachFig(head, f, if (f->inherits("vevent") && ((vevent *)f)->code == 0 /* click event */) {
    if (eventcnt==0) clickevent = (vevent *)f;
    eventcnt ++;
  });
  if (eventcnt==0) { //イベント図形がないので追加しよう
    head.compaction();
    if (head.size() > 0 && head[0]) { // 少なくとも1つはオブジェクトがあるとき
      clickevent = (vevent *)BookBase::defaultTool->objects[1]->clone();
      withheadevent = 0;
      clickevent->setPos(head[0]->pos);
      head.append(clickevent);
    } else {
      // オブジェクトがない
      return;
    }
  } else if (eventcnt >= 2) { // 2つ以上イベント図形がある
    head.remove(clickevent); // 最初にあるイベントを消す
    checkEvent(); // もう一回チェック
    return;
  } else if (eventcnt == 1) {
    head.remove(clickevent);
    head.compaction();
    double lmax = 100000;
    double dis;
    int nearx = -1;
    int cnt=0;
    ForEachFig(head, f, 
	       f->highlight = false; 
	       if ((dis = f->distance(clickevent)) < lmax) { nearx=cnt; lmax = dis; } cnt++;);
    if (nearx>=0) {
      //      withheadevent = nearx;
      clickevent->pos = head[nearx]->pos;
      head[nearx]->highlight = true;
    }
    head.append(clickevent);
  }


  vobjectvector events;
  // body のチェック
  events.clear();

  // ボディにある全イベントを集める．
  ForEachFig(body, f, f->highlight = false;if (f->inherits("vevent") && ((vevent *)f)->code == 0 /* click event */) {
    events.append(f); 
  });

  ForEachFig(events, f, body.remove(f)); // まずボディから削除

  body.compaction(); // 圧縮

  bodyevent.clear(); // イベントにくっついているオブジェクト

  ForEachFig(events, o, 
    // click イベント図形は他のオブジェクトの中心に移動する
    double lmax = 100000;
    double dis;
    vobject *nearx = 0;
    ForEachFig(body, f, if ((dis = f->distance(o)) < lmax) { nearx=f; lmax = dis; });
    if (nearx) {
      nearx->highlight = true;
      o->pos = nearx->pos;
      bodyevent.append(nearx);
    }
    body.append(o);
  );
  dirty = TRUE;

  // reply: headevent, bodyevent
  // headevent == 0 のときはエラーが含まれているルール だから無視する
}

void vrule::checkEvent() {
  // あたらしいフォーマット

  // ここでやること
  // head にはちゃんとclickイベントがあるか
  // body にclickイベントがあったら，それは浮いていないか
  // いずれのイベント図形も必ずvectorの最後に置かれること

  withheadevent = 0;
  ForEachFig(head, f, if (f->highlight) withheadevent = f);
retry:
  ForEachFig(head, f, if (f->inherits("vevent")) {
    vobject *clickevent = f;
    head.remove(clickevent);
    double lmax = 100000;
    double dis;
    vobject *nearf = 0;
    ForEachFig(head, f, 
	       f->highlight = false;
	       if ((dis = f->distance(clickevent)) < lmax) { nearf=f; lmax = dis; });
    if (nearf) {
      if (withheadevent) {
	withheadevent->highlight = false;
      }
      nearf->highlight = true;
      withheadevent = nearf;
    }
    delete clickevent;
    goto retry;
  });
  head.compaction();  
  if (!withheadevent) {
    if (head.size() > 0 && head[0]) {
      head[0]->highlight = true;
      withheadevent = head[0];
    }
  }

nexttry:
  ForEachFig(body, f, if (f->inherits("vevent")) {
    vobject *clickevent = f;
    body.remove(clickevent);
    double lmax = 100000;
    double dis;
    vobject *nearf = 0;
    ForEachFig(body, f, 
	       if ((dis = f->distance(clickevent)) < lmax) { nearf=f; lmax = dis; });
    if (nearf) nearf->highlight = true;
    delete clickevent;
    goto nexttry;
  });

  body.compaction();

  dirty = TRUE;
}

void vrule::calcConstraint() {
  if (!dirty) return; // よごれてなければ計算しない
  if (!withheadevent) return; // エラーが含まれているルール

  dirty = FALSE;
  if (hc) { delete hc; hc = 0; }
  if (bc) { delete bc; bc = 0; }
  if (hbc){ delete hbc; hbc = 0; }

  int i,j;
  headnoevent.clear();
  headnoeventcount = 0;
  headAttribute.clear();
  headAttributeCount = 0;
  for (j=0;j<head.size();j++) {
    vobject *f1 = head[j];
    if (f1 && vobject::isRegal(f1)) {
		if (f1->inherits("rotvobject")) {
			headnoevent.append(f1);
			headnoeventcount++;
		} else if (f1->inherits("vAttribute")) {
			headAttribute.append(f1);
			headAttributeCount++;
		}
	}
  }

  int bodycount = body.size();

  double totalWeight=0;
  for (j=0;j<headnoeventcount;j++) {
    vobject *f1 = headnoevent[j];
    if (f1) {
      for (i=0;i<j;i++) {
	vobject *f2 = headnoevent[i];
	if (f2) {
	  hc = constraint::create(f1, j, f2, i, hc);
	  totalWeight += hc->weight;
	}
      }
    }
  }

  {
    constraint *c = hc;
    while (c) {
      c->totalWeight = totalWeight;
      c = c->next;
    }
  }


  for (j=0;j<headnoeventcount;j++) {
    vobject *f1 = headnoevent[j];
    if (f1) {
      constraint *identical=0;
      double minDist = 100000000;
      for (i=0;i<bodycount;i++) {
        vobject *f2 = body[i];
        if (f2) {
	  hbc = constraint::create(f1, j, f2, i, hbc);
	  if (f1->sameKind(f2)) {
	    // ヘッドとボディで同一関係とみなせるものを探す(移動あつかい)
	    if (hbc->distance < minDist) {
	      identical = hbc;
	      minDist = hbc->distance;
	    }
	  }
	}
      }
      if (identical) { // みつかった
	identical->identical = true;
      }
    }
  }

  for (j=0;j<bodycount;j++) {
    vobject *f1 = body[j];
    if (f1) {
      for (i=0;i<j;i++) {
	vobject *f2 = body[i];
	if (f2) {
	  bc = constraint::create(f1, j, f2, i, bc);
	}
      }
    }
  }
}

bool vrule::rmatch(vobject *o) {
  // 大雑把にマッチする可能性があるかどうか
  if (withheadevent) {
    return o->sameKind(withheadevent);
  } else {
    return FALSE;
  }
}

bool vrule::contains(QPoint p) {
  if (page && page->inherits("Palette")) return vobject::contains(p);
  return vobject::contains(p) || headRect.contains(p) || bodyRect.contains(p);
}

void vrule::click(Page *page, QPoint p) {
  if (headRect.contains(p)) {
    p = p - QPoint(headRect.left(), headRect.top());
    qDebug("head clicked %d %d", p.x(), p.y());
    ForEachFigRev(head, f,
		  if (f->contains(p)) {
		    if (f != withheadevent) {
		      f->highlight = true;
		      withheadevent->highlight = false;
		      withheadevent = f;
		      goto updateRule;
		    }
		  });
  } else if (bodyRect.contains(p)) {
    p = p - QPoint(bodyRect.left(), bodyRect.top());
    qDebug("body clicked %d %d", p.x(), p.y());
    ForEachFigRev(body, f, 
		  if (f->contains(p)) {
		    f->highlight = !f->highlight;
		    goto updateRule;
		  });
  }
updateRule:
  page->parent->mainBook->update(rect);
}

QPoint vrule::nearPos(QPoint p, double &angle) {
  if (bodyRect.contains(p)) {
    QPoint pp = p - bodyRect.topLeft();

    ForEachFig(head, f, if (f->distance(pp) < 5) {
      if (f->inherits("rotvobject")) {
	double da = angle - ((rotvobject *)f)->angle;
	if (da <0) da = -da;
	if (da < 10) angle = ((rotvobject *)f)->angle;
      }
      return p-pp+f->pos;
    });
  }
  return p;
}

void vrule::catchObject(vobject *obj) {
	if (withheadevent == 0) {
		body.deleteAll();

		vobject *o1, *o2;
		o1 = obj->clone();
		o2 = obj->clone();

		head.append(o1);
		body.append(o2);
		checkEvent();
		resize();
		updateRect();
	}
}
