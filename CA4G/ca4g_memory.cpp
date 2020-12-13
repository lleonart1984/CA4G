#include "ca4g_memory.h"

namespace CA4G {

	template<typename S>
	void gObj<S>::AddReference() {
		if (!counter)
			throw new CA4GException("Error referencing");

		InterlockedAdd(counter, 1);
	}

	template<typename S>
	void gObj<S>::RemoveReference() {
		if (!counter)
			throw new CA4GException("Error referencing");

		InterlockedAdd(counter, -1);
		if ((*counter) == 0) {
			delete _this;
			delete counter;
			//_this = nullptr;
			counter = nullptr;
		}
	}

}