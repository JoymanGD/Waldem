#include "wdpch.h"
#include "LayerStack.h"

namespace Waldem
{
    LayerStack::LayerStack()
    {
    }

    LayerStack::~LayerStack()
    {
        for (Layer* layer : Layers)
        {
            layer->OnDetach();
            delete layer;
        }
    }

    void LayerStack::PushLayer(Layer* layer)
    {
        Layers.emplace(Layers.begin() + LayerInsertIndex, layer);
        LayerInsertIndex++;
    }

    void LayerStack::PushOverlay(Layer* overlay)
    {
        Layers.emplace_back(overlay);
    }

    void LayerStack::PopLayer(Layer* layer)
    {
        auto it = std::find(Layers.begin(), Layers.end(), layer);

        if(it != Layers.end())
        {
			layer->OnDetach();
            Layers.erase(it);
            LayerInsertIndex--;
        }
    }

    void LayerStack::PopOverlay(Layer* overlay)
    {
        auto it = std::find(Layers.begin(), Layers.end(), overlay);

        if(it != Layers.end())
        {
			overlay->OnDetach();
            Layers.erase(it);
        }
    }
}
