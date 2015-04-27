#include "fuzzyrewriting.h"
#include "constraint.h"
#include "vobject.h"
#include "page.h"
#include "vrule.h"
#include "vmisc.h"
#include "solver.h"
#include "bookbase.h"
#include "book.h"
#include "binder.h"
#include "tool.h"

#ifdef Q_OS_WIN32
#include "DirectInput.h"
#endif

#include <math.h>
#include <qsound.h>
#include <qdatetime.h>

mContext::mContext() {
  maxPos.clear();
  maxScore = -100000;
}

FuzzyRewriting::FuzzyRewriting(Page *page, vobject *obj, int coordM) {

  coordMode = coordM;
  //  fprintf(stderr, "rewriting\n");

  mcontext.clear();
  visited.clear();
  click = obj;
  nclick = page->objects.findRef(obj);

  top = page;

  // click が原点になるように平行移動する
  double wrap;
  int iwrap = vAttribute::count(page->objects, vAttribute::wrap);
  wrap= 1+iwrap*0.2;

  wW = page->parent->pageWidth() * wrap;
  wH = page->parent->pageHeight() * wrap;

  if (coordMode == 0) {
    int x1 = click->pos.x()-wW/2;
    int y1 = click->pos.y()-wH/2;
    int x2 = click->pos.x()+wW/2;
    int y2 = click->pos.y()+wH/2;
    
    ForEachFig(page->objects, f, 
	       if (f->inherits("rotvobject")) {
		 int px = f->pos.x();
		 int py = f->pos.y();
		 if (px < x1) px += wW;
		 else if (px >= x2) px -= wW;
		 if (py < y1) py += wH;
		 else if (py >= y2) py -= wH;
		 f->pos = QPoint(px, py);
	       });
  }

  collectRule(page);

  double maxScore = -100000;
  int max = -1;
  for (int i=0;i<mcontext.size();i++) {
    mcontext[i].matching();
    //qDebug("match %d %f", i, mcontext[i].maxScore);
    if (maxScore < mcontext[i].maxScore) {
      maxScore = mcontext[i].maxScore;
      max = i;
    }
  }
  //qDebug("found rule %f", maxScore);
  /*
  if (max >=0) {
    for (int i=0;i<mcontext[max].maxPos.size();i++) {
      qDebug("pos %d:%d", i, mcontext[max].maxPos[i]);
    }
    // vrule *r = mcontext[max].rule;
    //    if (r->hbc) r->hbc->dump();
    
  }
  */
  if (max >= 0) mcontext[max].rewriting(this);

  // もとにもどす

  if (coordMode == 0) {
    ForEachFig(page->objects, f, 
	       if (f->inherits("rotvobject")) {
		 int px = f->pos.x();
		 int py = f->pos.y();
		 if (px < 0) px += wW;
		 else if (px >= wW) px -= wW;
		 if (py < 0) py += wH;
		 else if (py >= wH) py -= wH;
		 f->pos = QPoint(px, py);f->updateRect();
	       });
  } else if (coordMode == 1) {
    ForEachFig(page->objects, f, 
	       if (f->inherits("rotvobject")) {
		 int px = f->pos.x();
		 int py = f->pos.y();
		 if ((px < 0) || (px >= wW) || (py < 0) || (py >= wH)) 
		   page->objects.remove(f);
	       });
  }

  //fprintf(stderr, "done\n");
}

void FuzzyRewriting::collectRule(Page *p) {
  if (p && visited.find(p)<0) {
    // 2度たどらない
    visited.append(p);
    ForEachFig(p->objects, f, 
	       if (f->inherits("vrule")) {
		 if (((vrule *)f)->rmatch(click)) {
		   // ラフにマッチする
		   // スクリーニング
		   mContext mc;
		   mc.rule = (vrule *)f;
		   mc.page = top;
		   mc.click = nclick;
		   mcontext.append(mc);
		   // qDebug("collect %d %d", f, p);
		 }
	       } else if (f->inherits("vPointer")) {
		 Page *np = gBookBase->searchPage(((vPointer *)f)->pageName);
		 if (np) collectRule(np);
	       }
	       );
  }
}

void mContext::matching() {
  if (rule) rule->calcConstraint();
  //  fprintf(stderr, "matching %d %d\n",rule, page, click);
  int matchLimit = gBookBase->confMatchLimit; 
  QValueVector<int> pos;

  maxPos.clear();
  maxScore = -100000;

  // アトリビュートを探す
  double attributeScore = 0;  

  if (rule->headAttributeCount>0) {
      int dir = 0;
      int btn = 0;
#ifdef Q_OS_WIN32
		gDirectInput->input();
		dir = gDirectInput->direction;
		btn = gDirectInput->button;
#endif
	  ForEachFig(rule->headAttribute, f, 
		vAttribute *va = (vAttribute *)f;
//	  qDebug("attri %d %d %d", dir, btn, va->value);

          if (va->value>=vAttribute::game1 && va->value<=vAttribute::gameW) {
	  switch(va->value) {
          case vAttribute::game1:
		  if (!(btn & 1)) return;
		  break;
	  case vAttribute::game2:
		  if (!(btn & 2)) return;
		  break;
	  case vAttribute::gameW:
		  if (dir!=1) return;
		  break;
	  case vAttribute::gameN:
		  if (dir!=2) return;
		  break;
	  case vAttribute::gameE:
		  if (dir!=3) return;
		  break;
	  case vAttribute::gameS:
		  if (dir!=4) return;
		  break;
          }
	  attributeScore += 10;
          } else if (va->value==vAttribute::random) {
              int r = rand();
              if (r % 3 == 1) attributeScore+=10;
          }
	  ); 
//	  qDebug("attriscore %f", attributeScore);
  }

  int hsize = rule->headnoeventcount;
  int tsize = page->objects.size();
  pos.resize(hsize);
  //  qDebug("hsize %d  tsize %d", hsize, tsize);

  int u = 0;
  pos[0] = 0;
  while (1) {
  start1:
    vobject *ho = rule->headnoevent[u];
    vobject *f = page->objects[pos[u]];
    //        fprintf(stderr, "u:%d %d  pos[u]:%d %d click:%d\n", u, ho, pos[u],f, click);

    if (ho == rule->withheadevent && pos[u] != click) goto next1;


    { for (int i=0;i<u;i++) {
      if (pos[i] == pos[u]) goto next1;
    } }
    if (!f) goto next1;
    if (!f->sameKind(ho)) goto next1;
    u++;
    if (u == hsize) { // 組合せができた
      double score = 0;
      constraint *c = rule->hc;
      if (c) {
	while (c) {
	  score += c->check(page->objects[pos[c->id1]], page->objects[pos[c->id2]]);
	  c = c->next;
	}
      } else {
	score = -100; // 制約がないから絶対マッチ
      }

      if (1 || hsize==1) {
	// 絶対角も追加しようか
	for (int i=0;i<hsize;i++) {
	  vobject *ho = rule->headnoevent[i];
	  vobject *f = page->objects[pos[i]];
	  double a = ((rotvobject *)ho)->angle- ((rotvobject *)f)->angle;
	  a = constraint::normAngle(a)/hsize;
	  if (a < 0) a = -a;
	  score -= (a/30);
	}
      }
	  // 属性の追加
	  score += attributeScore;
//            qDebug("score %f", score);
      if (score > maxScore) {
	maxPos = pos;
	maxScore = score;
      }

	  if (matchLimit-- < 0) return;
      /*
      fprintf(stderr, "match %f:", score);
      { for (int i=0;i<hsize;i++) {
	fprintf(stderr, "%d ", pos[i]);
      }}
      fprintf(stderr, "\n");
      */
      u--;
    } else {
      pos[u] = 0;
      goto start1;
    }
  next1:
    pos[u]++;
    if (pos[u] >= tsize) {
      u--;
      if (u < 0) {
	return;
      }
      goto next1;
    }
  }
}

void mContext::rewriting(FuzzyRewriting *fr) {
  int size = rule->body.size();

  int wW = fr->wW;
  int wH = fr->wH;
  int coordMode = fr->coordMode;
  vPointer *gotoPage = 0;
  int navigation = 0;
  
  QRect updateRect;


  { for (int i=0;i<size;i++) {
    vobject *f = rule->body[i];
    if (f) {
      if (f->inherits("rotvobject")) {
	vobject *n = f->clone();
	double mx = 0;
	double my = 0;
	double dx = 0;
	double dy = 0;
	double pi = 3.1415926545/180;
	double base = 0;
	constraint *c = rule->hbc;
	vobject *samePosition = 0;
	vobject *sameAngle = 0;
	//fprintf(stderr, "gen1 ");
	while (c) {
	  if (c->id2 == i) {
	    int id = maxPos[c->id1];
	    if (id>=0) {
	      vobject *obj = page->objects[id];
	      if (c->distance == 0) {
		// 同じ場所にあるものを覚えておく
		samePosition = obj;
		if (c->dangle1 == c->dangle2) {
		  // 角度も同じ
		  sameAngle = obj;
		}
	      }

	      c->generate(obj, n);
	      
	      double bb = c->weight;
	      bb = bb*(1+c->pangle1)*(1+c->pangle2);
	      
	      mx += n->pos.x()*bb;
	      my += n->pos.y()*bb;
	      double ang = ((rotvobject *)n)->angle;
	      dx += cos(ang*pi)*bb;
	      dy += sin(ang*pi)*bb;
	      base += bb;
	      //fprintf(stderr, "(%d %d %f %f) ", n->pos.x(), n->pos.y(), ((rotvobject *)n)->angle, bb);
	    }
	  }
	  c = c->next;
	}
	
	dx = dx/base;
	dy = dy/base;

	if (samePosition) {
	  n->pos = samePosition->pos;
	} else {
	  n->pos = QPoint((int)(mx/base), (int)(my/base));
	}

	if (sameAngle) {
	  ((rotvobject *)n)->angle = ((rotvobject *)sameAngle)->angle;
	} else {
	  ((rotvobject *)n)->angle = constraint::angle(dx, dy);
	}
	// グリッドの処理など
	page->restrict((rotvobject *)n);

	
	//fprintf(stderr, ":  %d %d %f\n", n->pos.x(), n->pos.y(), ((rotvobject *)n)->angle);      
	n->setup();
	updateRect = updateRect.unite(n->rect);
	page->addObject(n);

	if (f->highlight) { // イベント
	  int wait;
	  wait = vAttribute::count(rule->body, vAttribute::wait)*gBookBase->confWait+
	    vAttribute::count(rule->body, vAttribute::longwait)*gBookBase->confLWait;

	  if (wait==0) wait = gBookBase->confWait*gBookBase->confWaitCnt; // デフォルト
	  if (coordMode == 0) {
	    int px = n->pos.x();
	    int py = n->pos.y();
	    if (px < 0) px += wW;
	    else if (px >= wW) px -= wW;
	    if (py < 0) py += wH;
	    else if (py >= wH) py -= wH;
	    n->pos = QPoint(px, py);
	    page->enqueue(n, wait);
	  } else if (coordMode == 1) {
	    int px = n->pos.x();
	    int py = n->pos.y();
	    if (px >= 0 && px < wW &&
		py >= 0 && py < wH) {
	      page->enqueue(n, wait);
	    }
	  }
	}
      } else if (f->inherits("vPointer")) {
	gotoPage = (vPointer *)f;
      } else if (f->inherits("vSound")) {
	((vSound *)f)->click(page, f->pos);
      } else if (f->isAttribute(vAttribute::backward)) {
		  navigation = -1;
	  } else if (f->isAttribute(vAttribute::forward)) {
		  navigation = 1;
	  }
    }
  }}

  // 削除
  {for (int i=0;i<maxPos.size();i++) {
    int id = maxPos[i];
    if (id>=0) {
      vobject *obj = page->objects[id];
      if (obj) {
	updateRect = updateRect.unite(obj->rect);
	page->removeObject(id);
	delete obj;
      }
    }
  }}

  if (page->parent) {
    if (gotoPage) {
      Page *newpg = gBookBase->searchPage(gotoPage->pageName);
      if (newpg) {
	page->parent->mainBook->gotoPage(newpg);
      }
	} else if (navigation) {
		if (navigation==1) gBookBase->book->visibleForward();
		if (navigation==-1) gBookBase->book->visibleBackward();
    } else {
      page->parent->mainBook->update(updateRect);
    }
  }

  {
    QTime t = QTime::currentTime();
    //qDebug("rewriting done %d %d", t.second(), t.msec());
    //page->dumpqueue();
  }
}
