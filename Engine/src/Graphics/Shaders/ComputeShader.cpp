#include "ComputeShader.h"
#include "../../Core/System/Logger.h"
#include <windows.h>

#pragma comment(lib, "d3dcompiler.lib")

// Helper function to convert wide string to narrow string
std::string WideToNarrow(const WCHAR* wideStr)
{
    if (!wideStr) return "";
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";
    
    std::string narrowStr(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &narrowStr[0], size, nullptr, nullptr);
    return narrowStr;
}

std::string GetCurrentWorkingDirectory()
{
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer) == 0)
    {
        return "Failed to get current directory";
    }
    return std::string(buffer);
}

ComputeShader::ComputeShader()
    : m_computeShader(nullptr)
    , m_computeShaderBuffer(nullptr)
{
}

ComputeShader::~ComputeShader()
{
}

bool ComputeShader::Initialize(ID3D11Device* device, HWND hwnd, const WCHAR* filename, const char* entryPoint)
{
    // Validate input parameters
    if (!device || !filename || !entryPoint)
    {
        LOG_ERROR("Invalid parameters passed to ComputeShader::Initialize");
        return false;
    }
    
    LOG("Initializing compute shader: " + WideToNarrow(filename));
    
    bool result = CompileComputeShader(device, hwnd, filename, entryPoint);
    if (!result)
    {
        LOG_ERROR("Failed to compile compute shader: " + WideToNarrow(filename));
        return false;
    }

    // Create the compute shader from the buffer
    HRESULT hr = device->CreateComputeShader(m_computeShaderBuffer->GetBufferPointer(), m_computeShaderBuffer->GetBufferSize(), nullptr, &m_computeShader);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to create compute shader from compiled buffer");
        LOG_ERROR("HRESULT: " + std::to_string(hr));
        return false;
    }
    
    if (!m_computeShader)
    {
        LOG_ERROR("CreateComputeShader succeeded but returned null shader");
        return false;
    }
    
    LOG("Compute shader initialized successfully: " + WideToNarrow(filename));
    LOG("Compute shader buffer size: " + std::to_string(m_computeShaderBuffer->GetBufferSize()) + " bytes");
    LOG("Compute shader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_computeShader)));
    return true;
}

void ComputeShader::Shutdown()
{
    if (m_computeShader)
    {
        m_computeShader->Release();
        m_computeShader = nullptr;
    }

    if (m_computeShaderBuffer)
    {
        m_computeShaderBuffer->Release();
        m_computeShaderBuffer = nullptr;
    }
}

bool ComputeShader::CompileComputeShader(ID3D11Device* device, HWND hwnd, const WCHAR* filename, const char* entryPoint)
{
    HRESULT result;
    ID3D10Blob* errorMessage;

    // Check if file exists
    DWORD fileAttributes = GetFileAttributes(filename);
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        LOG_ERROR("Compute shader file not found: " + WideToNarrow(filename));
        LOG_ERROR("Current working directory: " + GetCurrentWorkingDirectory());
        return false;
    }
    else
    {
        LOG("Compute shader file found: " + WideToNarrow(filename));
    }

    LOG("Compiling compute shader: " + WideToNarrow(filename) + " with entry point: " + entryPoint);

    // Compile the compute shader
    result = D3DCompileFromFile(filename, nullptr, nullptr, entryPoint, "cs_5_0", 
                               D3D10_SHADER_ENABLE_STRICTNESS, 0, &m_computeShaderBuffer, &errorMessage);
    if (FAILED(result))
    {
        // If it failed, check if it was a file not found error
        if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            LOG_ERROR("Compute shader file not found during compilation: " + WideToNarrow(filename));
            LOG_ERROR("Current working directory: " + GetCurrentWorkingDirectory());
        }
        else
        {
            LOG_ERROR("Failed to compile compute shader: " + WideToNarrow(filename));
        }
        
        // If there was an error message, output it
        if (errorMessage)
        {
            OutputShaderErrorMessage(errorMessage, hwnd, filename);
            errorMessage->Release();
        }
        return false;
    }

    LOG("Compute shader compiled successfully: " + WideToNarrow(filename));
    return true;
}

void ComputeShader::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, const WCHAR* shaderFilename)
{
    char* compileErrors;
    unsigned long long bufferSize, i;
    std::ofstream fout;

    compileErrors = (char*)(errorMessage->GetBufferPointer());
    bufferSize = errorMessage->GetBufferSize();

    fout.open("shader-error.txt");
    for (i = 0; i < bufferSize; i++)
    {
        fout << compileErrors[i];
    }
    fout.close();

    MessageBox(hwnd, L"Error compiling compute shader. Check shader-error.txt for message.", shaderFilename, MB_OK);
}

void ComputeShader::Dispatch(ID3D11DeviceContext* context, UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
{
    // Add null check to prevent crash
    if (!m_computeShader)
    {
        LOG_ERROR("Compute shader is null - cannot dispatch");
        return;
    }
    
    LOG("ComputeShader::Dispatch - Setting compute shader and dispatching");
    LOG("ComputeShader::Dispatch - Compute shader pointer: " + std::to_string(reinterpret_cast<uintptr_t>(m_computeShader)));
    LOG("ComputeShader::Dispatch - Thread groups: " + std::to_string(threadGroupCountX) + "x" + std::to_string(threadGroupCountY) + "x" + std::to_string(threadGroupCountZ));
    
    context->CSSetShader(m_computeShader, nullptr, 0);
    context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    
    LOG("ComputeShader::Dispatch - Dispatch completed");
}

void ComputeShader::SetShaderResourceView(ID3D11DeviceContext* context, UINT slot, ID3D11ShaderResourceView* srv)
{
    LOG("ComputeShader::SetShaderResourceView - Setting SRV at slot " + std::to_string(slot) + " with pointer: " + std::to_string(reinterpret_cast<uintptr_t>(srv)));
    context->CSSetShaderResources(slot, 1, &srv);
}

void ComputeShader::SetUnorderedAccessView(ID3D11DeviceContext* context, UINT slot, ID3D11UnorderedAccessView* uav)
{
    LOG("ComputeShader::SetUnorderedAccessView - Setting UAV at slot " + std::to_string(slot) + " with pointer: " + std::to_string(reinterpret_cast<uintptr_t>(uav)));
    context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void ComputeShader::SetConstantBuffer(ID3D11DeviceContext* context, UINT slot, ID3D11Buffer* buffer)
{
    LOG("ComputeShader::SetConstantBuffer - Setting CB at slot " + std::to_string(slot) + " with pointer: " + std::to_string(reinterpret_cast<uintptr_t>(buffer)));
    context->CSSetConstantBuffers(slot, 1, &buffer);
} 