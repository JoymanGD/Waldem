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
        Layers.AddAt(layer, LayerInsertIndex);
        LayerInsertIndex++;
    }

    void LayerStack::PushOverlay(Layer* overlay)
    {
        Layers.Add(overlay);
    }

    void LayerStack::PopLayer(Layer* layer)
    {
        auto it = Layers.Find(layer);

        if(it)
        {
			layer->OnDetach();
            Layers.Remove(it);
            LayerInsertIndex--;
        }
    }

    void LayerStack::PopOverlay(Layer* overlay)
    {
        auto it = Layers.Find(overlay);

        if(it)
        {
			overlay->OnDetach();
            Layers.Remove(it);
        }
    }
}
