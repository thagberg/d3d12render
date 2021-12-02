#include "DescriptorAllocator.h"

constexpr uint32_t kMaxSamplers = 2048;
constexpr uint32_t kMaxRTVs = 16;
constexpr uint32_t kMaxMisc = 4056;

namespace hvk
{
	namespace render
	{
		bool operator > (const HeapPointer& lhs, const HeapPointer& rhs)
		{
			return lhs.mCpuHandle.ptr > rhs.mCpuHandle.ptr;
		}

		bool operator < (const HeapPointer& lhs, const HeapPointer& rhs)
		{
			return lhs.mCpuHandle.ptr < rhs.mCpuHandle.ptr;
		}

		bool operator == (const HeapPointer& lhs, const HeapPointer& rhs)
		{
			return lhs.mCpuHandle.ptr == rhs.mCpuHandle.ptr;
		}

		bool operator >= (const HeapPointer& lhs, const HeapPointer& rhs)
		{
			return lhs == rhs || lhs > rhs;
		}

		bool operator <= (const HeapPointer& lhs, const HeapPointer& rhs)
		{
			return lhs == rhs || lhs < rhs;
		}

		void _fillHeapPointers(HeapWrapper& heap, uint32_t numDescriptors)
		{
			heap.mBegin = heap.mCurrent = {
				heap.mHeap->GetGPUDescriptorHandleForHeapStart(),
				heap.mHeap->GetCPUDescriptorHandleForHeapStart()
			};

			auto gpuEnd = heap.mBegin.mGpuHandle.ptr + heap.mDescriptorSize * numDescriptors;
			auto cpuEnd = heap.mBegin.mCpuHandle.ptr + heap.mDescriptorSize * numDescriptors;

			heap.mEnd.mGpuHandle.ptr = gpuEnd;
			heap.mEnd.mCpuHandle.ptr = cpuEnd;
		}

		DescriptorAllocator::DescriptorAllocator(const RenderContext& renderCtx)
		{
			{
				D3D12_DESCRIPTOR_HEAP_DESC miscDesc = {};
				miscDesc.NumDescriptors = kMaxMisc;
				miscDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				miscDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				HRESULT hr = renderCtx.GetDevice()->CreateDescriptorHeap(&miscDesc, IID_PPV_ARGS(&mMiscHeap.mHeap));
				assert(SUCCEEDED(hr));

				mMiscHeap.mDescriptorSize = renderCtx.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				assert(mMiscHeap.mDescriptorSize != 0);

				_fillHeapPointers(mMiscHeap, miscDesc.NumDescriptors);
			}

			{
				D3D12_DESCRIPTOR_HEAP_DESC samplerDesc = {};
				samplerDesc.NumDescriptors = kMaxSamplers;
				samplerDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				samplerDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				HRESULT hr = renderCtx.GetDevice()->CreateDescriptorHeap(&samplerDesc, IID_PPV_ARGS(&mSamplerHeap.mHeap));
				assert(SUCCEEDED(hr));

				mSamplerHeap.mDescriptorSize = renderCtx.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
				assert(mSamplerHeap.mDescriptorSize != 0);

				_fillHeapPointers(mSamplerHeap, samplerDesc.NumDescriptors);
			}

			{
				D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
				rtvDesc.NumDescriptors = kMaxRTVs;
				rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				HRESULT hr = renderCtx.GetDevice()->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&mRTVHeap.mHeap));
				assert(SUCCEEDED(hr));

				mRTVHeap.mDescriptorSize = renderCtx.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				assert(mRTVHeap.mDescriptorSize != 0);

				// non-shader-visible heaps don't have a GPU handle
				mRTVHeap.mBegin.mCpuHandle = mRTVHeap.mHeap->GetCPUDescriptorHandleForHeapStart();
				mRTVHeap.mCurrent = mRTVHeap.mBegin;

				auto cpuEnd = mRTVHeap.mBegin.mCpuHandle.ptr + mRTVHeap.mDescriptorSize * kMaxRTVs;

				mRTVHeap.mEnd.mCpuHandle.ptr = cpuEnd;
			}
		}

		DescriptorAllocator::~DescriptorAllocator()
		{

		}

		DescriptorAllocation DescriptorAllocator::AllocDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, uint32_t num)
		{
			// descriptor tables must be contiguous, so if an allocation exceeds end - current, we have to move to
			// the beginning of the ring. This will leave some wasted space in the ring.
			DescriptorAllocation newAllocation = {};

			HeapWrapper* heap = nullptr;
			switch (descriptorType)
			{
			case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
				heap = &mMiscHeap;
				break;
			case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				heap = &mSamplerHeap;
				break;
			case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
				heap = &mRTVHeap;
				break;
			default:
				assert(false);
			}

			newAllocation.mPointer = heap->mCurrent;

			auto increment = heap->mDescriptorSize * num;
			auto newPtr = heap->mCurrent + increment;

			if (newPtr >= heap->mEnd) {
				newPtr = heap->mBegin;
			}

			newAllocation.mPointer = newPtr;
			heap->mCurrent = newAllocation.mPointer + increment;

			return newAllocation;
		}
	}
}