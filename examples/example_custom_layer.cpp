#include "../Process/CProcess.hpp"

// "Temporary solution"
#ifdef _WIN32
#include "../Process/Platform/Windows/WinapiLink.hpp"
#endif

using namespace Azoth;

#ifdef _WIN32

// Override a specific platform method from the default backend
class MySuperiorReadImplementation : public WinapiLink
{
    //Override Read method with your own code
    bool read(uint64_t addr, size_t size, void* buffer) const override
	{
        std::cout << "Custom read backend invoked!\n";

		size_t bytesRead;
		if (ReadProcessMemory(this->_hProcess, (LPVOID)addr, buffer, size, (SIZE_T*)&bytesRead) && bytesRead != 0)
			return true;

		return setError(EPlatformError::InternalError, GetLastError());
	}
};

#endif

// Alternatively, provide a completely custom platform backend
class MySuperiorImplementation : public IPlatformLink
{
    //.....
    bool initialize() override
    {
        return true;
    }

	bool isInitialized() const override
    {
        return true;
    }

	bool attach(uint32_t procID) override
    {
        _attached = true;
        return true;
    }

	bool isAttached() const override
    {
       return _attached;
    }

	void detach() override
    {
        _attached = false;
    }
private:
    bool _attached = false;
};

int main()
{
#ifdef _WIN32

    // Using a partially overridden backend
    CProcess process1(std::make_unique<MySuperiorReadImplementation>());

#endif

    // Using a fully custom backend
    CProcess process2(std::make_unique<MySuperiorImplementation>());

    return 0;
}