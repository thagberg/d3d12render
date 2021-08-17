#include "ShaderService.h"

#include <assert.h>

namespace hvk
{
	namespace render
	{
		namespace shader
		{
			//std::shared_ptr<ShaderService> sInstance = nullptr;
			//ComPtr<IDxcCompiler> ShaderService::sCompiler = nullptr;
			//ComPtr<IDxcUtils> ShaderService::sUtils = nullptr;

			constexpr auto kDllName = L"dxcompiler.dll";
			constexpr auto kDxcCreateInstanceName = "DxcCreateInstance";

			ShaderException::ShaderException(std::string message)
				: mMessage(message)
			{

			}

			const char* ShaderException::what() const
			{
				return mMessage.c_str();
			}

			ShaderService::ShaderService()
				: mCompilerDll()
				, mCreateInstanceFunction()
				, mCompiler(nullptr)
				, mUtils(nullptr)
				, mIncludeHandler(nullptr)
			{
				mCompilerDll = LoadLibraryW(kDllName);
				assert(mCompilerDll != nullptr);
				if (mCompilerDll == nullptr)
				{
					throw ShaderException("Could not load dxcompiler.dll");
				}

				mCreateInstanceFunction = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(mCompilerDll, kDxcCreateInstanceName));
				assert(mCreateInstanceFunction != nullptr);
				if (mCreateInstanceFunction == nullptr)
				{
					throw ShaderException("Failed to find DxcCreateInstanceProc");
				}

				mCreateInstanceFunction(CLSID_DxcCompiler, __uuidof(&mCompiler), &mCompiler);
				assert(mCompiler != nullptr);
				if (mCompiler == nullptr)
				{
					throw ShaderException("Failed to create DXC Compiler instance");
				}

				mCreateInstanceFunction(CLSID_DxcUtils, __uuidof(&mUtils), &mUtils);
				assert(mUtils != nullptr);
				if (mUtils == nullptr)
				{
					throw ShaderException("Failed to create DXC Utils instance");
				}

				mUtils->CreateDefaultIncludeHandler(&mIncludeHandler);
				if (mIncludeHandler == nullptr)
				{
					throw ShaderException("Failed to create default Include Handler");
				}
			}

			std::shared_ptr<ShaderService> ShaderService::Initialize()
			{
				if (sInstance != nullptr)
				{
					return sInstance;
				}

				sInstance = std::shared_ptr<ShaderService>(new ShaderService());

				return sInstance;
			}

			HRESULT ShaderService::CompileShader(const ShaderDefinition& definition, IDxcBlob** outBlob)
			{
				HRESULT hr = S_OK;

				uint32_t codePage = 0;
				IDxcBlobEncoding* pShaderText(nullptr);
				IDxcOperationResult* result;

				hr = mUtils->LoadFile(definition.mFilename.c_str(), &codePage, &pShaderText);
				if (SUCCEEDED(hr))
				{
					hr = mCompiler->Compile(pShaderText, definition.mFilename.c_str(), definition.mEntrypoint.c_str(), definition.mProfile.c_str(), nullptr, 0, nullptr, 0, mIncludeHandler.Get(), &result);
					if (SUCCEEDED(hr))
					{
						result->GetStatus(&hr);
						if (SUCCEEDED(hr))
						{
							hr = result->GetResult(outBlob);
						}
						else
						{
							ComPtr<IDxcBlobEncoding> errors;
							hr = result->GetErrorBuffer(&errors);
							if (SUCCEEDED(hr) && errors)
							{
								auto errorMessage = static_cast<const char*>(errors->GetBufferPointer());
								throw ShaderException(errorMessage);
							}
						}
					}
					else
					{
						throw ShaderException("Failed to compile shader");
					}
				}
				else
				{
					throw ShaderException("Failed to load shader file");
				}

				return hr;
			}
		}
	}
}