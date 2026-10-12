#ifndef DRAWABLE_H
#define DRAWABLE_H
#include <QPainterPath>
#include <QPainter>
#include <QImage>
#include <vector>

struct Drawable
{
    virtual ~Drawable() = default;
    virtual void draw(QPainter* painter) = 0;
};

// Bruh this is soo confusing.. make this better..

struct ImageDrawable : public Drawable {
    QImage image;
    QPointF point;
    void draw(QPainter* painter) override {
        painter->drawImage(point, image);
    }
};

struct PathDrawable : public Drawable {
    QPainterPath path;
    void draw(QPainter* painter) override {
        painter->setBrush(QColor(0,0,255));
        painter->drawPath(path);
    }
};


struct TextDrawable : public Drawable {
    PathDrawable frontText;
    ImageDrawable frontTextBase;
    std::vector<ImageDrawable> frontTextShadows;

    std::vector<ImageDrawable> backTextLayers;
    void draw(QPainter* painter) override {
        painter->save();
        for (auto & backLayer : backTextLayers) {
            backLayer.draw(painter);
        }
        if (!frontText.path.isEmpty())
            painter->setClipPath(frontText.path, Qt::IntersectClip);
        frontTextBase.draw(painter);

        painter->restore();
        // frontText.draw(painter);
    }
};


#endif // DRAWABLE_H
