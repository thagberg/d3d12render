#pragma once

//#include "dxcapi.h"
#include <memory>
#include <exception>
#include <string>
#include <vector>

#include <Windows.h>
#include <wrl/client.h>
#include "dxc/dxcapi.h"
//#include <d3d12shader.h>
#include "dxc/d3d12shader.h"

using namespace Microsoft::WRL;

namespace hvk 
{ 
	namespace render
	{
		namespace shader
		{
			class ShaderException : public std::exception
			{
			public:
				ShaderException(std::string message);
				virtual const char* what() const throw() override;
			private:
				std::string mMessage;
			};

			struct ShaderDefinition
			{
				std::wstring mFilename;
				std::wstring mEntrypoint;
				std::wstring mProfile;
			};

			class ShaderService
			{
			public:
				static std::shared_ptr<ShaderService> Initialize();
				HRESULT CompileShader(const ShaderDefinition& definition, IDxcBlob** outBlob);
			private:
				ShaderService();

				HMODULE mCompilerDll;
				DxcCreateInstanceProc mCreateInstanceFunction;
				ComPtr<IDxcCompiler> mCompiler;
				ComPtr<IDxcUtils> mUtils;
				ComPtr<IDxcIncludeHandler> mIncludeHandler;

			};

			static std::shared_ptr<ShaderService> sInstance = nullptr;
		}
	}
}
