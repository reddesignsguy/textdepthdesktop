#ifndef QTTOPHOTOSHOPAPI_H
#define QTTOPHOTOSHOPAPI_H

#include <QPainterPath>
#include <QImage>
#include <QMetaType>
#include <unordered_map>
#include <vector>
#include <PhotoshopAPI.h>

using namespace NAMESPACE_PSAPI;

// Structures prepared by the Qt application.
struct Back {
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

struct Front {
    QPainterPath vectorMaskData;
    QImage baseLayer;
    std::vector<QImage> clippedLayers;
};

struct TextDepthUnit {
    Front front;
    Back back;
};

using TextDepthUnits = std::vector<TextDepthUnit>;
Q_DECLARE_METATYPE(TextDepthUnits)

// Structures consumed by PhotoshopAPI.
using PsApiRasterLayerInfo = std::unordered_map<Enum::ChannelID, std::vector<bpp8_t>>;
using PsApiVectorMask = Layer<bpp8_t>::VectorMask;

struct PsApiBackTextData {
    PsApiRasterLayerInfo baseLayer;
    std::vector<PsApiRasterLayerInfo> clippedLayers;
};

struct PsApiFrontTextData {
    PsApiVectorMask vectorMaskData;
    PsApiRasterLayerInfo baseLayer;
    std::vector<PsApiRasterLayerInfo> clippedLayers;
};

struct PsApiTextData {
    PsApiFrontTextData front;
    PsApiBackTextData back;
};

using PsApiLayersDto = std::vector<PsApiTextData>;

// Convert Qt text data to PhotoshopAPI text data.
PsApiLayersDto qtToPsApi (TextDepthUnits qtInfo);

// Convert bridge data back to Qt, using the same dimensions for all raster layers.
// Empty channel maps produce null images. Nonempty maps require RGB channels;
// alpha is optional (defaults to opaque). Invalid dimensions/channel sizes throw
// std::invalid_argument. Vector paths use qtToPsApi's per-endpoint handle encoding.
TextDepthUnits psApiToQt(const PsApiLayersDto& psInfo, int width, int height);

#endif // QTTOPHOTOSHOPAPI_H
