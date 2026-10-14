#ifndef BACKGENERATION_H
#define BACKGENERATION_H
#include "TextDepthUnit.h"
#include <QSize>

QPainterPath backOutline(const Front &front, QPointF relativePosition, QPointF relativeSize);
QPainterPath backWireframe(const Front &front, QPointF relativePosition, QPointF relativeSize);

// Generates independent, transparent raster layers without mutating a unit.
Back generateBack(const Front &front, QSize canvasSize,
                  QPointF relativePosition, QPointF relativeSize);
#endif
