// vfigure.h
#ifndef VFIGURE_H
#define VFIGURE_H

#include "vobject.h"

class vfigure : public rotvobject {
  Q_OBJECT
    public:
  virtual vobject *clone();
};

#endif
