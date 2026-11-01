#ifndef BACKPREVIEW_H
#define BACKPREVIEW_H
#include <QPainterPath>
#include <QUuid>
#include <QMetaType>
#include <QHash>

// Transient viewport message, never part of a document or export snapshot.
struct BackPreview {
    QUuid unitId;
    QPainterPath wireframe;
    QHash<QUuid, QPainterPath> wireframes;
};
Q_DECLARE_METATYPE(BackPreview)
#endif
