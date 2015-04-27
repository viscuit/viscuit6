#include "constraint.h"
#include "vobject.h"
#include "bookbase.h"
#include <math.h>

double constraint::normAngle(double angle) {
  if (angle > 180) angle -= 360;
  if (angle < -180) angle += 360;
  if (angle > 180) angle -= 360;
  if (angle < -180) angle += 360;
  return angle;
}

double constraint::angle(double dx, double dy) {
  if (dx == 0) {
    if (dy > 0) return 90;
    return 270;
  }
  double ang = atan(dy/dx)*180/3.1415926545;
  if (dx < 0) ang += 180;
  return normAngle(ang);
}

double constraint::diff(double zero, double a) {
  return exp(-zero*zero/a);
}
double constraint::step(double x, double a, double b) {
  double c = (a+b)/2;
  if (x < a) return 0;
  if (x < c) return (x-a)*(x-a)/(2*(c-a)*(c-a));
  if (x < b) return 1-(x-b)*(x-b)/(2*(b-c)*(b-c));
  return 1;
}

constraint * constraint::create(vobject *o1, int i1, vobject *o2, int i2, constraint *n) {
  if (o1->inherits("rotvobject") && o2->inherits("rotvobject")) {
    constraint *c = new constraint;
    c->id1 = i1;
    c->id2 = i2;
    QPoint dpoint = o1->pos - o2->pos;
    c->distance = sqrt((double)(dpoint.x()*dpoint.x() + dpoint.y()*dpoint.y()));
    double ang = angle(dpoint.x(), dpoint.y());
    c->dangle1 = normAngle(((rotvobject *)o1)->angle - ang);
    c->dangle2 = normAngle(((rotvobject *)o2)->angle - ang);

    double d;

    /*
    if (c->distance==0) d=100;
    else d=10/c->distance;
    
    d = d*(o1->overlapped(o2) ? 10 : 1);
    */

    d = diff(c->distance, 50*50)*20+diff(c->distance, 5*5)*100;

    c->weight = d;

    int size1 = o1->rect.width()+o1->rect.height();
    int size2 = o2->rect.width()+o2->rect.height();

    if (gBookBase->confMatchParam1==0) {
      double dd = gBookBase->confMatchParam2*gBookBase->confMatchParam2;
      c->pangle1 = 1-diff(size1,dd);
      c->pangle2 = 1-diff(size2,dd);
    } else {
      c->pangle1 = step(size1, gBookBase->confMatchParam1, gBookBase->confMatchParam2);
      c->pangle2 = step(size2, gBookBase->confMatchParam1, gBookBase->confMatchParam2);
    }
    //    qDebug("diff %d %f   %d %f", size1, c->pangle1, size2, c->pangle2);


    c->maxscore = 400;
    c->totalWeight = 1;
    c->next = n;
    return c;
  }
  return n;
}

double constraint::check(vobject *o1, vobject *o2) {
  if (o1->inherits("rotvobject") && o2->inherits("rotvobject")) {
    /*
    qDebug("check");
    o1->dump();
    o2->dump();
    */
    QPoint dpoint = o1->pos - o2->pos;
    double dx = dpoint.x();
    double dy = dpoint.y();
    return checkSub(dx, dy, ((rotvobject *)o1)->angle, ((rotvobject *)o2)->angle, 0.5, 10000, 100, 900);
  }
  return 0;
}

double constraint::checkSub(double dx, double dy, double angle1, double angle2,
			    double d_param1, double d_param2, double d_val, double a_param) {


  double x_distance = sqrt(dx*dx + dy*dy);

  double ang = angle(dx, dy);

  double da1 = normAngle(angle1-ang);
  double da2 = normAngle(angle2-ang);

  double dd = distance-x_distance;
  double dm = (distance+x_distance)/2;
  double pi = 3.141592654;
  double pp = pi/180;
  double dd1 = normAngle(dangle1-da1)*pp*dm*pangle1;
  double dd2 = normAngle(dangle2-da2)*pp*dm*pangle2;
  double da = normAngle((dangle1-da1)-(dangle2-da2))*pp*50*pangle1*pangle2;

  double s1 = (dd <0) ? -dd: dd;
  double s2 = (dd1 <0) ? -dd1:dd1;
  double s3 = (dd2 <0) ? -dd2:dd2;
  double s4 = (da <0) ? -da:da;

  //  s1 = diff(s1, 1000000);
  //  s2 = diff(s2, 1000000);
/*

  qDebug("    cs %f %f %f %f   (%f %f %f)  (%f %f) %f %f %f %f\n",
	  dx, dy, angle1, angle2, dangle1, dangle2, distance, pangle1, pangle2, s1, s2, s3, s4);
*/
  return -(s1+s2+s3+s4)*(weight/totalWeight);
//  return (s1+s2)/2;

  /*



  double score = 0;
  double ang;
  double x_distance = sqrt(dx*dx + dy*dy);
  double dist = distance-x_distance;
  double sdist1 = diff(dist, d_param2);
  double sdist2 = diff(dist, d_param1*(distance + x_distance)*(distance + x_distance)+0.1);
  sdist2 = 1;

  score += sdist1*300;
		
  ang = angle(dx, dy);

  // 距離が小さいと極めて不安定になる
  double angerror = (1-exp(-distance*distance/100)) * (1-exp(-x_distance*x_distance/100));			

  {
    double x_da1 = angle1 - ang;
    double dd1 = normAngle(dangle1-x_da1);
    
    double sang1 = diff(dd1 * angerror, a_param) * sdist2;
    score +=  sang1 * 100;

    fprintf(stderr, "a1 %f ", sang1);

  }
  {
    double x_da2 = angle2 - ang;
    double dd2 = normAngle(dangle2-x_da2);
    double sang2 = diff(dd2 * angerror, a_param) * sdist2;
    score +=  sang2 * 100;

    fprintf(stderr, "a2 %f ", sang2);

  }
  {
    double dd = normAngle((dangle1-dangle2)-(angle1-angle2));
    double sang0 = diff(dd, a_param)* sdist2;
    score += sang0 * 100;

    fprintf(stderr, "a0 %f ", sang0);
  }

  fprintf(stderr, "s %f\n", score/600);

  score = (score/maxscore) * weight/totalWeight;
  return score;
  */
}

void constraint::generate(vobject *ref, vobject *gen) {
  rotvobject *o1 = (rotvobject *)ref;
  rotvobject *o2 = (rotvobject *)gen;

  double ang = (o1->angle - dangle1)*3.1415926545/180;
  double dx = distance * cos(ang);
  double dy = distance * sin(ang);

  o2->pos = o1->pos - QPoint((int)dx, (int)dy);
  o2->angle = normAngle(o1->angle-dangle1+dangle2);
  if (identical) {
    //オブジェクトが動いた場合

    o2->serial = o1->serial;
    o2->posDelta = o2->pos - o1->pos;
  }
}

void constraint::dump() {
  fprintf(stderr, "%d %d: %f %f %f %f %f %f\n", id1, id2, dangle1, dangle2, distance,
	  weight, totalWeight, maxscore);
  if (next) next->dump();
}
