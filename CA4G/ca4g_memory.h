#ifndef CA4G_MEMORY_H
#define CA4G_MEMORY_H

#include "ca4g_errors.h"

namespace CA4G {

	template<typename S>
	class gObj {
		friend S;
		//template<typename T> friend class list; // I dont like it... :(
		template<typename T> friend class gObj;

	private:
		S* _this;
		volatile long* counter;

		void AddReference();

		void RemoveReference();

	public:
		gObj() : _this(nullptr), counter(nullptr) {
		}
		gObj(S* self) : _this(self), counter(self ? new long(1) : nullptr) {
		}

		inline bool isNull() const { return _this == nullptr; }

		// Copy constructor
		gObj(const gObj<S>& other) {
			this->counter = other.counter;
			this->_this = other._this;
			if (!isNull())
				AddReference();
		}

		template <typename Subtype>
		gObj(const gObj<Subtype>& other) {
			this->counter = other.counter;
			this->_this = (S*)other._this;
			if (!isNull())
				AddReference();
		}

		gObj<S>& operator = (const gObj<S>& other) {
			if (!isNull())
				RemoveReference();
			this->counter = other.counter;
			this->_this = other._this;
			if (!isNull())
				AddReference();
			return *this;
		}

		bool operator == (const gObj<S>& other) {
			return other._this == _this;
		}

		bool operator != (const gObj<S>& other) {
			return other._this != _this;
		}

		template<typename A>
		auto& operator[](A arg) {
			return (*_this)[arg];
		}

		~gObj() {
			if (!isNull())
				RemoveReference();
		}

		//Dereference operator
		S& operator*()
		{
			return *_this;
		}
		//Member Access operator
		S* operator->()
		{
			return _this;
		}

		template<typename T>
		gObj<T> Dynamic_Cast() {
			gObj<T> obj;
			obj._this = dynamic_cast<T*>(_this);
			obj.counter = counter;
			obj.AddReference();
			return obj;
		}

		template<typename T>
		gObj<T> Static_Cast() {
			gObj<T> obj;
			obj._this = static_cast<T*>(_this);
			obj.counter = counter;
			obj.AddReference();
			return obj;
		}

		operator bool() const {
			return !isNull();
		}
	};

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

#endif