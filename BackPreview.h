#ifndef BACKPREVIEW_H
#define BACKPREVIEW_H
#include <QPainterPath>
#include <QUuid>
#include <QMetaType>

// Transient viewport message, never part of a document or export snapshot.
struct BackPreview {
    QUuid unitId;
    QPainterPath wireframe;
};
Q_DECLARE_METATYPE(BackPreview)
#endif
