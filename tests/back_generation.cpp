// Include the implementation to exercise coverage and plane ownership with
// exact geometry, independently of font outlines and vanishing-point sorting.
#include "BackGeneration.cpp"
#include <iostream>

namespace {
void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main()
{
    try {
        // Fractional boundaries must preserve RGB and represent area in alpha.
        const std::vector<Quad> fractional{{{2.25, 2}, {10.25, 2}, {2.25, 12}, {10.25, 12}}};
        const auto base = createBackLayerBase(fractional, 20, 70, Qt::white);
        const auto shadow = createBackLayerShadows(fractional, 20, 70);
        require(qAlpha(base.pixel(2, 5)) == 191, "Expected 3/4 coverage at left edge");
        require(qAlpha(base.pixel(10, 5)) == 64, "Expected 1/4 coverage at right edge");
        require(qRed(base.pixel(2, 5)) == 255, "Transparent samples darkened white edge");
        require(qAlpha(base.pixel(1, 5)) == 0, "Coverage leaked outside geometry");
        for (int y = 0; y < base.height(); ++y)
            for (int x = 0; x < base.width(); ++x)
                require(qAlpha(base.pixel(x, y)) == qAlpha(shadow.pixel(x, y)), "Base/shadow coverage differs");

        // A rectangle partitioned by a diagonal must have no alpha seam. The
        // diagonal crosses band boundaries and separates black/white shading.
        const std::vector<Quad> stitched{
            {{2, 2}, {18, 2}, {2, 66}, {2, 66}},
            {{18, 2}, {18, 66}, {2, 66}, {2, 66}}
        };
        const auto stitchedBase = createBackLayerBase(stitched, 20, 70, Qt::white);
        const auto stitchedShadow = createBackLayerShadows(stitched, 20, 70);
        int blended = 0;
        for (int y = 2; y < 66; ++y) {
            for (int x = 2; x < 18; ++x) {
                require(qAlpha(stitchedBase.pixel(x, y)) == 255, "Shared edge has transparent seam");
                const QRgb p = stitchedShadow.pixel(x, y);
                require(qAlpha(p) == 255, "Shadow shared edge has transparent seam");
                if (qRed(p) > 0 && qRed(p) < 255) ++blended;
            }
        }
        require(blended >= 60, "Internal shadow boundary was not smoothed");
        require(qRed(stitchedShadow.pixel(3, 3)) == 0, "Dark interior changed");
        require(qRed(stitchedShadow.pixel(17, 64)) == 255, "Light interior changed");

        const auto empty = createBackLayerBase({}, 20, 70, Qt::white);
        require(qAlpha(empty.pixel(0, 0)) == 0 && qAlpha(empty.pixel(19, 69)) == 0,
                "Empty layer must be transparent");
        const std::vector<Quad> clipped{{{-20, -20}, {40, -20}, {-20, 80}, {40, 80}}};
        const auto full = createBackLayerBase(clipped, 20, 70, Qt::white);
        require(full.pixel(0, 0) == qRgb(255, 255, 255) && full.pixel(19, 69) == qRgb(255, 255, 255),
                "Canvas clipping or last band failed");
        Front front;
        front.vectorMaskData.addEllipse(QRectF(10, 10, 30, 40));
        const auto generated = generateBack(front, QSize(80, 100), QPointF(13, 21), QPointF(.8, .8));
        require(generated.baseLayer.size() == QSize(80, 100), "Generation changed canvas size");
        int partial = 0;
        for (int y = 0; y < 100; ++y)
            for (int x = 0; x < 80; ++x) {
                const int alpha = qAlpha(generated.baseLayer.pixel(x, y));
                require(alpha == qAlpha(generated.shadowLayer.pixel(x, y)), "Curve coverage mismatch");
                if (alpha > 0 && alpha < 255) ++partial;
            }
        require(partial > 20, "Curved silhouette has no antialiasing");
        std::cout << "Back generation coverage tests passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
