#include "qtToPhotoshopAPI.h"
#include <stdexcept>
#include <iostream>

using Image = ImageLayer<bpp8_t>;
using Group = GroupLayer<bpp8_t>;

static void check(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

static std::shared_ptr<Group> group(const std::string &name)
{
    Layer<bpp8_t>::Params params{};
    params.name = name;
    return std::make_shared<Group>(params);
}

static std::shared_ptr<Image> image(int value, bool front = false, bool clipped = false)
{
    Layer<bpp8_t>::Params params{};
    params.name = "raster";
    params.width = 2;
    params.height = 2;
    params.center_x = 2;
    params.center_y = 2;
    params.clipping_mask = clipped;
    PsApiRasterLayerInfo channels;
    for (auto id : {Enum::ChannelID::Red, Enum::ChannelID::Green, Enum::ChannelID::Blue})
        channels[id] = std::vector<bpp8_t>(4, value);
    auto layer = std::make_shared<Image>(channels, params);
    if (front) {
        using Point = Image::PathPoint;
        std::vector<Point> points = {
            Point({1,1}, {1,1}, {1,1}, false),
            Point({3,1}, {3,1}, {3,1}, false),
            Point({3,3}, {3,3}, {3,3}, false)
        };
        layer->m_vectorMask = std::make_unique<PsApiVectorMask>(
            std::vector<Image::SubPath>{Image::SubPath(points, true), Image::SubPath(true)}, false);
    }
    return layer;
}

int main()
{
    LayeredFile<bpp8_t> file(Enum::ColorMode::RGB, 5, 4);
    auto outer = group("outer");
    auto targets = group("targets");
    auto first = group("first");
    auto second = group("second");
    // A clipped layer can itself have a vector mask; it is still a clip.
    first->layers({image(90, true, true), image(80, false, true), image(70, true),
                   image(60, false, true), image(50)});
    auto cropped = image(20);
    cropped->center_x(0); // Extends one pixel outside the canvas.
    second->layers({image(30, true), cropped});
    targets->layers({first, image(255), second}); // Ignore direct non-group children.
    outer->layers({targets});
    file.layers({image(255, true), outer}); // Ignore unrelated document layers.
    PsApiData extracted;
    auto qt = psApiToQt(file, extracted);
    check(extracted.size() == 2 && qt.size() == 2, "must extract both pairs only");
    check(extracted[0].name == "first" && extracted[1].name == "second", "group order/names");
    check(qt[0].front.clippedLayers.size() == 2 && qt[0].back.clippedLayers.size() == 1,
          "clipping base association");
    check(qt[0].front.clippedLayers[0].pixelColor(1,1).red() == 80 &&
          qt[0].front.clippedLayers[1].pixelColor(1,1).red() == 90, "clip paint order");
    check(qt[0].back.baseLayer.pixelColor(1,1).red() == 50, "existing back preserved");
    check(qt[0].front.baseLayer.size() == QSize(5,4) &&
          qt[0].front.baseLayer.pixelColor(0,0).alpha() == 0 &&
          qt[0].front.baseLayer.pixelColor(1,1).alpha() == 255, "offset and absent alpha");
    check(qt[1].back.baseLayer.pixelColor(0,1).red() == 20 &&
          qt[1].back.baseLayer.pixelColor(1,1).alpha() == 0, "off-canvas clipping");
    check(!qt[0].front.vectorMaskData.isEmpty(), "vector mask conversion");
    second->layers({image(40, false, true), image(30, true)});
    qt = psApiToQt(file, extracted);
    check(!qt[1].front.baseLayer.isNull() && qt[1].back.baseLayer.isNull() &&
          qt[1].front.clippedLayers.size() == 1, "front-only group with clips");
    second->layers({image(40, false, true), image(20)});
    qt = psApiToQt(file, extracted);
    check(qt[1].front.baseLayer.isNull() && qt[1].front.vectorMaskData.isEmpty() &&
          !qt[1].back.baseLayer.isNull() && qt[1].back.clippedLayers.size() == 1,
          "back-only group with clips");
    second->layers({});
    qt = psApiToQt(file, extracted);
    check(qt[1].front.baseLayer.isNull() && qt[1].back.baseLayer.isNull(),
          "empty group satisfies maximum counts");
    for (bool front : {true, false}) {
        second->layers({image(30, front), image(20, front)});
        bool rejected = false;
        try { psApiToQt(file, extracted); } catch (const std::invalid_argument &) { rejected = true; }
        check(rejected && extracted.size() == 2 && extracted[1].front.baseLayer.empty() &&
              extracted[1].back.baseLayer.empty(), "duplicate base preserves previous extraction");
    }
    file.layers({});
    bool rejected = false;
    try { psApiToQt(file, extracted); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "missing targets must fail safely");
    std::cout << "PSD import tests passed\n";
}
