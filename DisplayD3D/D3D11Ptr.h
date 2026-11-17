/*
	D3D11Ptr.h
	RAII wrapper for D3D11 COM resources (similar to Microsoft::WRL::ComPtr)
	Provides automatic resource management to prevent leaks
*/

#ifndef D3D11PTR_H
#define D3D11PTR_H

#include <d3d11.h>
#include <utility> // for std::swap

//----------------------------------------------------------------------------
// D3D11Ptr - RAII wrapper for D3D11 COM interfaces
//----------------------------------------------------------------------------

template<typename T>
class D3D11Ptr
{
public:
	// Default constructor - null pointer
	D3D11Ptr() : m_ptr(nullptr) {}

	// Constructor from raw pointer (takes ownership)
	explicit D3D11Ptr(T* ptr) : m_ptr(ptr) {}

	// Destructor - automatically releases resource
	~D3D11Ptr()
	{
		Reset();
	}

	// Copy constructor - increases reference count
	D3D11Ptr(const D3D11Ptr& other) : m_ptr(other.m_ptr)
	{
		if (m_ptr)
			m_ptr->AddRef();
	}

	// Copy assignment
	D3D11Ptr& operator=(const D3D11Ptr& other)
	{
		if (this != &other)
		{
			Reset();
			m_ptr = other.m_ptr;
			if (m_ptr)
				m_ptr->AddRef();
		}
		return *this;
	}

	// Move constructor
	D3D11Ptr(D3D11Ptr&& other) noexcept : m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}

	// Move assignment
	D3D11Ptr& operator=(D3D11Ptr&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}
		return *this;
	}

	// Assignment from raw pointer
	D3D11Ptr& operator=(T* ptr)
	{
		if (m_ptr != ptr)
		{
			Reset();
			m_ptr = ptr;
		}
		return *this;
	}

	// Release current resource and set to null
	void Reset()
	{
		if (m_ptr)
		{
			m_ptr->Release();
			m_ptr = nullptr;
		}
	}

	// Release current resource and replace with new one
	void Reset(T* ptr)
	{
		if (m_ptr != ptr)
		{
			Reset();
			m_ptr = ptr;
		}
	}

	// Detach pointer without releasing (caller takes ownership)
	T* Detach()
	{
		T* ptr = m_ptr;
		m_ptr = nullptr;
		return ptr;
	}

	// Get raw pointer (for passing to functions)
	T* Get() const
	{
		return m_ptr;
	}

	// Get address of pointer (for out parameters)
	// WARNING: This releases the current resource!
	T** GetAddressOf()
	{
		Reset();
		return &m_ptr;
	}

	// Get address without resetting (for QueryInterface, etc.)
	T** ReleaseAndGetAddressOf()
	{
		Reset();
		return &m_ptr;
	}

	// Member access
	T* operator->() const
	{
		return m_ptr;
	}

	// Dereference
	T& operator*() const
	{
		return *m_ptr;
	}

	// Boolean conversion
	explicit operator bool() const
	{
		return m_ptr != nullptr;
	}

	// Comparison operators
	bool operator==(const D3D11Ptr& other) const
	{
		return m_ptr == other.m_ptr;
	}

	bool operator!=(const D3D11Ptr& other) const
	{
		return m_ptr != other.m_ptr;
	}

	bool operator==(T* ptr) const
	{
		return m_ptr == ptr;
	}

	bool operator!=(T* ptr) const
	{
		return m_ptr != ptr;
	}

	// Swap
	void Swap(D3D11Ptr& other)
	{
		std::swap(m_ptr, other.m_ptr);
	}

private:
	T* m_ptr;
};

//----------------------------------------------------------------------------
// Common D3D11 type aliases
//----------------------------------------------------------------------------

using D3D11DevicePtr = D3D11Ptr<ID3D11Device>;
using D3D11ContextPtr = D3D11Ptr<ID3D11DeviceContext>;
using D3D11SwapChainPtr = D3D11Ptr<IDXGISwapChain>;
using D3D11Texture2DPtr = D3D11Ptr<ID3D11Texture2D>;
using D3D11RenderTargetViewPtr = D3D11Ptr<ID3D11RenderTargetView>;
using D3D11DepthStencilViewPtr = D3D11Ptr<ID3D11DepthStencilView>;
using D3D11ShaderResourceViewPtr = D3D11Ptr<ID3D11ShaderResourceView>;
using D3D11BufferPtr = D3D11Ptr<ID3D11Buffer>;
using D3D11VertexShaderPtr = D3D11Ptr<ID3D11VertexShader>;
using D3D11PixelShaderPtr = D3D11Ptr<ID3D11PixelShader>;
using D3D11InputLayoutPtr = D3D11Ptr<ID3D11InputLayout>;
using D3D11RasterizerStatePtr = D3D11Ptr<ID3D11RasterizerState>;
using D3D11BlendStatePtr = D3D11Ptr<ID3D11BlendState>;
using D3D11DepthStencilStatePtr = D3D11Ptr<ID3D11DepthStencilState>;
using D3D11SamplerStatePtr = D3D11Ptr<ID3D11SamplerState>;
using DXGIFactoryPtr = D3D11Ptr<IDXGIFactory>;
using DXGIAdapterPtr = D3D11Ptr<IDXGIAdapter>;
using DXGIOutputPtr = D3D11Ptr<IDXGIOutput>;
using DXGIDevicePtr = D3D11Ptr<IDXGIDevice>;

#endif // D3D11PTR_H
