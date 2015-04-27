
#include "vfigure.h"
#include "palette.h"
#include <qpainter.h>


vobject *vfigure::clone() {
  vfigure *f = new vfigure;
  f->itbl = itbl;
  f->id = id;
  f->pos = pos;
  f->angle = angle;
  f->mat = mat;
  f->updateRect();
  return f;
}
