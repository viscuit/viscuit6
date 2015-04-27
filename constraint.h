// constraint.h

#ifndef CONSTRAINT_H
#define CONSTRAINT_H

class vobject;
class constraint {
public:
  int id1, id2;

  double dangle1, dangle2, distance;

  double pangle1, pangle2;

  bool identical;

  static constraint * create(vobject *o1, int i1, vobject *o2, int i2, constraint *n);
  constraint *next;
  constraint() { identical=false; }
  ~constraint() {
    if (next) delete next;
  }
  double check(vobject *o1, vobject *o2);

  static double normAngle(double angle);
  static double angle(double dx, double dy);
  static double diff(double zero, double a);
  static double step(double x, double a, double b);
  double constraint::checkSub(double dx, double dy, double angle1, double angle2,
				     double d_param1, double d_param2, double d_val, double a_param);



  void generate(vobject *ref, vobject *gen); // refを与えて,この制約を満足するようにgenを動かす
  double weight;

  double totalWeight;
  double maxscore;
  
  void dump();
};

#endif
