// Solver

#include <qvaluevector.h>

class vvec : public QValueVector<double> {
public:
	vvec & operator+=(vvec &v);
	vvec & operator*=(double x);
};

class Solver {
public:
	Solver() { debug = FALSE; times = 10;}
 // 次の２つを与える．
	virtual double func(vvec &x);
 // デフォルトはn+1回funcを呼び出すやつ．
 // できれば，置き換えたほうが速い．
	virtual void dfunc(vvec &x, vvec &dx);

	double func1(double x, vvec &p, vvec &xi);

// 関数 func を最小化する
	bool solve(vvec &x);

// 補助関数 
	// p から xi の方向でfuncの最小値を求める
	double linmin(vvec &p, vvec &xi);
	void mnbrak(double &ax, double &bx, double &cx, 
				double &fa, double &fb,  double &fc,
				vvec &p, vvec &xi);
	double brent(double ax, double bx, double cx,
				 double tol, double &xmin,
				 vvec &p, vvec &xi);

	bool debug;
	uint times;

	bool newsolve(vvec &x);
};


class constraint;
class mContext;

class ConstraintSolver : public Solver {
public:
  mContext *mcontext;
  constraint *hc, *bc, *hbc;

  vvec vb, vh;
  vvec vbp, vhp;

  ConstraintSolver(mContext *mc);
  int step;
  virtual double func(vvec &x);
  virtual void dfunc(vvec &x, vvec &dx);
};
