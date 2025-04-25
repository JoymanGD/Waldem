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

        size_t Num() const { return Layers.Num(); }

        Layer*& operator[](size_t index) { return Layers[index]; }

        std::vector<Layer*>::iterator begin() { return Layers.begin(); }
        std::vector<Layer*>::iterator end() { return Layers.end(); }
    private:
        WArray<Layer*> Layers;
		unsigned int LayerInsertIndex = 0;
    };
}
