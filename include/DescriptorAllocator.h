#include "Render.h"
#include "RenderContext.h"

namespace hvk
{
	namespace render
	{
		struct HeapPointer
		{
			D3D12_GPU_DESCRIPTOR_HANDLE mGpuHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE mCpuHandle;

			HeapPointer operator + (uint32_t increment)
			{
				return HeapPointer{ mGpuHandle.ptr + increment, mCpuHandle.ptr + increment };
			}

		};
		bool operator > (const HeapPointer& lhs, const HeapPointer& rhs);
		bool operator < (const HeapPointer& lhs, const HeapPointer& rhs);
		bool operator == (const HeapPointer& lhs, const HeapPointer& rhs);
		bool operator >= (const HeapPointer& lhs, const HeapPointer& rhs);
		bool operator <= (const HeapPointer& lhs, const HeapPointer& rhs);

		struct DescriptorAllocation
		{
			HeapPointer mPointer;
			uint32_t mNumDescriptors;
		};

		struct HeapWrapper
		{
			ComPtr<ID3D12DescriptorHeap> mHeap = nullptr;
			HeapPointer mBegin;
			HeapPointer mCurrent;
			HeapPointer mEnd;
			uint32_t mDescriptorSize;
		};

		class DescriptorAllocator
		{
		public:
			DescriptorAllocator(const RenderContext&);
			~DescriptorAllocator();

			DescriptorAllocator(const DescriptorAllocator&) = delete;
			DescriptorAllocator(DescriptorAllocator&&) = delete;

			DescriptorAllocation AllocDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, uint32_t num);
		private:
			HeapWrapper mMiscHeap;
			HeapWrapper mRTVHeap;
			HeapWrapper mSamplerHeap;
		};
	}
}