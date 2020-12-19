#ifndef CA4G_ERRORS_H
#define CA4G_ERRORS_H

#include <comdef.h>
#include <stdexcept>

namespace CA4G {

	// Represents different error code this engine can report.
	enum CA4G_Errors {
		// Some error occurred.
		CA4G_Errors_Any,
		// The Pipeline State Object has a bad construction.
		CA4G_Errors_BadPSOConstruction,
		// The Signature has a bad construction
		CA4G_Errors_BadSignatureConstruction,
		// The image is in an unsupported format.
		CA4G_Errors_UnsupportedFormat,
		// Some shader were not found
		CA4G_Errors_ShaderNotFound,
		// Some initialization failed because memory was over
		CA4G_Errors_RunOutOfMemory,
		// Invalid Operation
		CA4G_Errors_Invalid_Operation,
		// Fallback raytracing device was not supported
		CA4G_Errors_Unsupported_Fallback
	};

	class CA4GException : public std::exception {
	public:
		CA4GException(const char* const message) :std::exception(message) {}

		static CA4GException FromError(CA4G_Errors error, const char* arg = nullptr, HRESULT hr = S_OK);
	};
}

#endif