#ifndef PHOTOSHOPWRITER_H
#define PHOTOSHOPWRITER_H

#include <QQmlEngine>
#include <PhotoshopAPI.h>
#include <QPointF>

class PhotoshopWriter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    PhotoshopWriter(QObject *parent = 0) : QObject(parent){};
    void write(std::string filename, std::vector<std::vector<QPointF>> subpaths);
};

#endif // PHOTOSHOPWRITER_H
