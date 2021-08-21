#pragma once

#include "ResourceManager.h"
#include "RenderContext.h"

#include <unordered_map>
#include <functional>
#include <vector>

namespace hvk
{
	namespace render
	{
		using ResourceMapping = std::unordered_map<ResourceHandle, ComPtr<ID3D12Resource>>;
		using RenderPassCallback = std::function<void(
			const RenderContext& renderCtx, 
			const ResourceMapping& inputMap, 
			const ResourceMapping& outputMap)>;

		class Framegraph
		{
		public:
			Framegraph();
			~Framegraph();

			void Insert(std::vector<ResourceHandle> passInputs, std::vector<ResourceHandle> passOutputs, const RenderPassCallback&);
		};
	}
}
