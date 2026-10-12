#include "PhotoshopWriter.h"
#include "qtToPhotoshopAPI.h"
#include <stdexcept>

void PhotoshopWriter::write(std::string filename, const RenderedTextDepthUnits &units)
{
    const auto data = qtToPsApi(units);
    QSize size;
    for (const auto &unit : units) {
        size = size.expandedTo(unit.front.baseLayer.size());
        if (unit.back) size = size.expandedTo(unit.back->baseLayer.size());
    }
    if (size.isEmpty()) throw std::invalid_argument("No rendered image to export.");
    LayeredFile<bpp8_t> document(Enum::ColorMode::RGB, size.width(), size.height());
    Layer<bpp8_t>::Params groupParams{};
    groupParams.name = "targets";
    auto targets = std::make_shared<GroupLayer<bpp8_t>>(groupParams);
    // PSD groups are top-to-bottom; snapshots are back-to-front.
    for (int i = static_cast<int>(units.size()) - 1; i >= 0; --i) {
        groupParams.name = "Layer " + std::to_string(units.size() - i);
        auto group = std::make_shared<GroupLayer<bpp8_t>>(groupParams);
        const auto addImage = [&](const PsApiRasterLayerInfo &channels, const QImage &image,
                                  const char *name, bool clipped, const PsApiVectorMask *mask) {
            if (image.isNull()) return;
            Layer<bpp8_t>::Params params{};
            params.name = name;
            params.width = image.width();
            params.height = image.height();
            params.center_x = image.width() / 2.0;
            params.center_y = image.height() / 2.0;
            params.clipping_mask = clipped;
            auto layer = std::make_shared<ImageLayer<bpp8_t>>(channels, params);
            if (mask) layer->set_vector_mask(*mask);
            group->layers().push_back(layer);
        };
        const auto &front = units[i].front;
        for (int j = static_cast<int>(front.clippedLayers.size()) - 1; j >= 0; --j)
            addImage(data[i].front.clippedLayers[j], front.clippedLayers[j], "Front clip", true, nullptr);
        addImage(data[i].front.baseLayer, front.baseLayer, "Front", false, &data[i].front.vectorMaskData);
        if (units[i].back) {
            const auto &back = *units[i].back;
            for (int j = static_cast<int>(back.clippedLayers.size()) - 1; j >= 0; --j)
                addImage(data[i].back.clippedLayers[j], back.clippedLayers[j], "Back clip", true, nullptr);
            addImage(data[i].back.baseLayer, back.baseLayer, "Back", false, nullptr);
        }
        targets->layers().push_back(group);
    }
    document.layers({targets});
    LayeredFile<bpp8_t>::write(std::move(document), filename);
}
