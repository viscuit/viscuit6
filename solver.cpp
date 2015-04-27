// Solver


#include "solver.h"
#include "page.h"
#include "constraint.h"
#include <math.h>

vvec & vvec::operator +=(vvec &v) {
  for (uint i=0;i<size();i++) {
    at(i) += v[i];
  }
  return *this;
}

vvec & vvec::operator *=(double x) {
  for (uint i=0;i<size();i++) {
    at(i) *= x;
  }
  return *this;
}


double Solver::func(vvec &x) {
  // ためしにやる関数
  double a = exp(-(x[0]-10)*(x[0]-10)/100);
  double b = exp(-(x[1]-5)*(x[1]-5)/100);
  double result = -a*b;
  
//	qDebug("func %f %f %f %f %f", x[0], x[1], a, b, result);
  return result;
}

void Solver::dfunc(vvec &x, vvec &dx) {
  uint i;
  uint size = x.size();
  double delta = 1;
  double f = func(x);
  for (i=0;i<size;i++) {
    x[i] += delta;
    double df = func(x);
    dx[i] = (df-f)/delta;
    x[i] -= delta;
  }
}

double Solver::func1(double x, vvec &p, vvec &xi) {
  vvec np(xi);
  np *= x;
  np += p;
  return func(np);	
}

bool Solver::solve(vvec &p) {
  uint size = p.size();
  uint its,j;
  vvec xi;
  xi.resize(size);
  double fp = func(p);
  dfunc(p, xi);
  xi*=-1;
  vvec g(xi), h(xi);
  
  for (its=0;its<times;its++) {
    double fret = linmin(p, xi);
    double ddd = 2.0*fabs(fret-fp) /(fabs(fret)+fabs(fp)+1.0e-10);
    
    if (debug) {
      if (p.size() >= 9) {
	qDebug("%d: %f %e < %e (%f %f %f %f %f %f %f %f %f)", its, fp, 2.0*fabs(fret-fp), (2.0e-10)*(fabs(fret)+fabs(fp)+1.0e-10), p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],p[8]);
      } else if (p.size() >= 6) {
	qDebug("%d: %f %e < %e (%f %f %f %f %f %f)", its, fp, 2.0*fabs(fret-fp), (2.0e-10)*(fabs(fret)+fabs(fp)+1.0e-10), p[0], p[1], p[2], p[3], p[4], p[5]);
      } else {
	qDebug("%d: %f/%e (%f %f %f)", its, fp, ddd, p[0], p[1], p[2]);
      }
    }
    if (2.0*fabs(fret-fp) < (2.0e-10)*(fabs(fret)+fabs(fp)+1.0e-10)) {
      //			qDebug("solve %d", its);
      return TRUE;
    }
    fp = func(p);
    dfunc(p, xi);
    double dgg=0.0, gg=0.0;
    for (j=0;j<size;j++) {
      gg += g[j]*g[j];
      dgg += (xi[j]+g[j])*xi[j];
    }
    if (gg == 0.0) {
      //			qDebug("solve %d", its);
      return TRUE;
    }
    double gam = dgg/gg;
    for (j=0;j<size;j++) {
      g[j] = -xi[j];
      xi[j]=h[j]=g[j]+gam*h[j];
    }
  }
  return FALSE;
}

#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d)
#define TINY 1.0e-20
#define GOLD 1.618034
#define GLIMIT 100

void Solver::mnbrak(double &ax, double &bx, double &cx,
		    double &fa, double &fb,
		    double &fc, vvec &p, vvec &xi) {
  fa = func1(ax, p, xi);
  fb = func1(bx, p, xi);
  if (fb > fa) {
    double dum;
    SHFT(dum, ax, bx, dum);
    SHFT(dum, fb, fa, dum);
  }
  cx = bx+GOLD*(bx-ax);
  fc = func1(cx, p, xi);
  while (fb > fc) {
    double r = (bx-ax)*(fb-fc);
    double q = (bx-cx)*(fb-fa);
    double tmp = fabs(q-r);
    if (tmp < TINY) tmp = TINY;
    if (q-r < 0) tmp = -tmp;
    double u = (bx-((bx-cx)*q-(bx-ax)*r)/(2.0*tmp));
    double ulim = bx-GLIMIT*(cx-bx);
    double fu;
    if ((bx-u)*(u-cx) > 0.0) {
      fu = func1(u, p, xi);
      if (fu < fc) {
	ax = bx;
	bx = u;
	fa = fb;
	fb = fu;
	return;
      } else if (fu > fb) {
	cx = u;
	fc = fu;
	return;
      }
      u = cx+GOLD*(cx-bx);
      fu = func1(u, p, xi);
    } else if (((cx-u)*u-ulim) > 0.0) {
      fu = func1(u, p, xi);
      if (fu < fc) {
	SHFT(bx, cx, u, cx+GOLD*(cx-bx));
	SHFT(fb, fc, fu, func1(u, p, xi));
      }
    } else if ((u-ulim)*(ulim-cx) >= 0.0) {
      u = ulim;
      fu = func1(u, p, xi);
    } else {
      u = cx+GOLD*(cx-bx);
      fu = func1(u, p, xi);
    }
    SHFT(ax, bx, cx, u);
    SHFT(fa, fb, fc, fu);
  }
}
#define ITMAX 100
#define CGOLD 0.3819660
#define ZEPS 1.0e-10


double Solver::brent(double ax, double bx, double cx,
		     double tol, double &xmin,
		     vvec &p, vvec &xi) {
  double a = (ax < cx ? ax : cx);
  double b = (ax > cx ? ax : cx);
  double x,w,v, fw, fv, fx;;
  double e = 0.0;
  double d,u,fu;
  x = w = v = bx;
  fw= fv= fx= func1(x, p, xi);
  for (uint iter = 0; iter < ITMAX; iter++) {
    double xm = 0.5*(a+b);
    double tol1 = tol*fabs(x)+ZEPS;
    double tol2 = 2.0*tol1;
    if (fabs(x-xm) <= tol2-0.5*(b-a)) {
      xmin = x;
      return fx;
    }
    if (fabs(e) > tol1) {
      double r = (x-w)*(fx-fv);
      double q = (x-v)*(fx-fw);
      double p = (x-v)*q-(x-w)*r;
      q = 2.0*(q-r);
      if (q > 0.0) p = -p;
      q = fabs(q);
      double etemp = e;
      e = d;
      if (fabs(p) >= fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x)) {
	e = (x >= xm ? a-x : b-x);
	d = CGOLD*e;
      } else {
	d = p/q;
	u = x+d;
	if (u-a < tol2 || b-u < tol2) {
	  d = tol1;
	  if (xm-x < 0) {
	    d = -d;
	  }
	}
      }
    } else {
      e = x > xm ? a-x : b-x;
      d = CGOLD*e;
    }
    u = (fabs(d) >= tol1 ? x+d : ((d < 0) ? x-tol1 : x+tol1));
    fu = func1(u, p, xi);
    if (fu <= fx) {
      if (u >= x) a=x; else b=x;
      SHFT(v,w,x,u);
      SHFT(fv,fw,fx,fu);
    } else {
      if (u < x) a=u; else b=u;
      if (fu <= fw || w==x) {
	v=w;
	w=u;
	fv=fw;
	fw=fu;
      } else if (fu <= fv || v == x || v == w) {
	v = u;
	fv = fu;
      }
    }
  }
  xmin = x;
  return fx;
}

double Solver::linmin(vvec &p, vvec &xi) {
  uint size = p.size();
  vvec pcom(p), xicom(xi);
  
  double ax = 0.0;
  double xx = 1.0;
  double bx, fa,fx,fb;
  mnbrak(ax, xx, bx, fa, fx, fb, p, xi);
  double xmin;
  double ret = brent(ax, xx, bx, 2.0e-4, xmin, p, xi);
  xi *= xmin;
  p += xi;
  return ret;
}

#include "vobject.h"
#include "fuzzyrewriting.h"
#include "vrule.h"

ConstraintSolver::ConstraintSolver(mContext *mc) : Solver() {

  mcontext = mc;
  vrule *r = mcontext->rule;

  int headsize = r->headnoeventcount;
  int bodysize = r->body.size();

  vh.resize(headsize*3);

  int i;

  for (i=0; i<headsize; i++) {
    int id = mc->maxPos[i];
    rotvobject *f = (rotvobject *)mc->page->objects[id];
    if (f) {
      vh[3*i+0] = f->pos.x();
      vh[3*i+1] = f->pos.y();
      vh[3*i+2] = f->angle;
    }
  }


  for (i=0;i<vh.size();i++) {
    fprintf(stderr, "%f ", vh[i]);
  }
fprintf(stderr, "vh\n");

  vb.resize(bodysize*3, 0);
  vbp.resize(bodysize*3,0); // これが1の変数だけ計算する

  // 制約のある変数を調べる．
  int ix, iy;
  vobject *ev = mc->page->objects[mc->click];
  ix = ev->pos.x();
  iy = ev->pos.y();

  hc = r->hc;
  bc = r->bc;
  hbc = r->hbc;

  constraint *c = hbc;
  while (c) {
    int i= c->id2; // ボディ側
    vbp[3*i+0] = 1;
    vbp[3*i+1] = 1;
    vbp[3*i+2] = 1;
    vb[3*i+0] = ix;
    vb[3*i+1] = iy;
    vb[3*i+2] = 0;
    c = c->next;
  }
  c = r->bc;
  while (c) {
    int i= c->id1;
    vbp[3*i+0] = 1;
    vbp[3*i+1] = 1;
    vbp[3*i+2] = 1;
    vb[3*i+0] = ix;
    vb[3*i+1] = iy;
    vb[3*i+2] = 0;

    i= c->id2;
    vbp[3*i+0] = 1;
    vbp[3*i+1] = 1;
    vbp[3*i+2] = 1;
    vb[3*i+0] = ix;
    vb[3*i+1] = iy;
    vb[3*i+2] = 0;
    c = c->next;
  }

  fprintf(stderr, "rot?\n");
  {for (int i=0;i<bodysize;i++) {
    if (vbp[3*i+2]==1) {
      int ang;
      int maxang=0;
      double min = 100000000;
      for (ang=0;ang<360;ang+=45) {
	vb[3*i+2] = ang;
	double a = func(vb);
	if (a < min) {
	  min = a;
	  maxang = ang;
	}
      }

      vb[3*i+2] = constraint::normAngle(maxang);
    }
  }}
  fprintf(stderr, "-rot\n");
}

double ConstraintSolver::func(vvec &x) {
  double result = 0;
  constraint *c = hbc;
  while (c) {
    int i1 = c->id1;
    int i2 = c->id2;

    double dx = vh[i1  ] - x[i2  ];
    double dy = vh[i1+1] - x[i2+1];

    double a1 = constraint::normAngle(vh[i1+2]);
    double a2 = constraint::normAngle(x[i2+2]);


    //    fprintf(stderr, "--- %f %f %f   %f %f %f\n", vh[i1], vh[i1+1],vh[i1+2], x[i2],x[i2+1],x[i2+2]);

    double p1,p2,p3,p4;
    p1=4;p2=80000;p3=300;p4=40000;
    result += c->checkSub(dx, dy, a1, a2, p1,p2,p3,p4);
    c = c->next;
  }


  //  fprintf(stderr, "func %f %f %f %f %f %f : %f\n",x[0],x[1],x[2],x[3],x[4],x[5], -result);

  return -result;
}

void ConstraintSolver::dfunc(vvec &x, vvec &dx) {
  uint i;
  uint size = x.size();
  double delta = 1;
  double f = func(x);
  for (i=0;i<size;i++) {
    if (vbp[i]==1) {
      x[i] += delta;
      double df = func(x);
      dx[i] = (df-f)/delta;
      x[i] -= delta;
    } else {
      dx[i] = 0;
    }
  }
}
/*
void ConstraintSolver::dfunc(vvec &x) {

}
*/


bool Solver::newsolve(vvec &p) {
  return false;
}
