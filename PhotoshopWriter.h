#ifndef PHOTOSHOPWRITER_H
#define PHOTOSHOPWRITER_H

#include <QQmlEngine>
#include "TextDepthUnit.h"
#include <string>

class PhotoshopWriter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:

    PhotoshopWriter(QObject *parent = 0) : QObject(parent){};
    void write(std::string filename, TextDepthUnits data);
};

#endif // PHOTOSHOPWRITER_H
