#ifndef BACKGENERATION_H
#define BACKGENERATION_H
#include "TextDepthUnit.h"
#include <QSize>

// Generates independent, transparent raster layers without mutating a unit.
Back generateBack(const Front &front, QSize canvasSize,
                  QPointF relativePosition, QPointF relativeSize);
#endif
