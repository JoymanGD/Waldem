#pragma once

#include "Waldem/Core.h"
#include "Layer.h"
#include <vector>

namespace Waldem
{
    class WALDEM_API LayerStack
    {
    public:
        LayerStack();
        ~LayerStack();

        void PushLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        void PopLayer(Layer* layer);
        void PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator begin() { return Layers.begin(); }
        std::vector<Layer*>::iterator end() { return Layers.end(); }
    private:
        std::vector<Layer*> Layers;
        std::vector<Layer*>::iterator LayerInsert;
    };
}
