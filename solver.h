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
 // ���̂Q��^����D
	virtual double func(vvec &x);
 // �f�t�H���g��n+1��func���Ăяo����D
 // �ł���΁C�u���������ق��������D
	virtual void dfunc(vvec &x, vvec &dx);

	double func1(double x, vvec &p, vvec &xi);

// �֐� func ���ŏ�������
	bool solve(vvec &x);

// �⏕�֐� 
	// p ���� xi �̕�����func�̍ŏ��l�����߂�
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
