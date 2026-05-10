#include "photoshopwriter.h"

void PhotoshopWriter::write(std::string filename, QtData qtData) {
    qDebug() << "Writing to: " << filename;

    auto psData = qtToPsApi(qtData);

    using namespace NAMESPACE_PSAPI;
    using VectorMask = ImageLayer<bpp8_t>::VectorMask;

    constexpr uint32_t width = 2000u;
    constexpr uint32_t height = 2000u;

    LayeredFile<bpp8_t> document = { Enum::ColorMode::RGB, width, height };
    for (const auto & text : psData)
    {
        VectorMask vmask = text.front.vectorMaskData;

        std::unordered_map <Enum::ChannelID, std::vector<bpp8_t>> channel_map;
        channel_map[Enum::ChannelID::Red] = std::vector<bpp8_t>(width * height, 255u);
        channel_map[Enum::ChannelID::Green] = std::vector<bpp8_t>(width * height, 0u);
        channel_map[Enum::ChannelID::Blue] = std::vector<bpp8_t>(width * height, 0u);

        ImageLayer<bpp8_t>::Params layer_params = {};
        layer_params.name = "Layer Red";
        layer_params.width = width;
        layer_params.height = height;
        layer_params.center_x = 32;
        layer_params.center_y = 32;

        auto layer = std::make_shared<ImageLayer<bpp8_t>>(
            std::move(channel_map),
            layer_params
            );

        layer->set_vector_mask(vmask);

        document.add_layer(layer);
    }


    LayeredFile<bpp8_t>::write(std::move(document), "TextDepthText.psd");
}
