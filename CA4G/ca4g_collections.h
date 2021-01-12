#ifndef CA4G_COLLECTIONS_H
#define CA4G_COLLECTIONS_H

namespace CA4G {

	class string {
		char* text;
		int length;

		int* references;

		void __AddReference() {
			if (!IsNull())
				(*references)++;
		}

		void __RemoveReference() {
			if (IsNull())
				return;

			(*references)--;

			if (*references == 0) {
				delete[] text;
				delete references;
			}
		}

	public:
		bool IsNull() const {
			return text == nullptr;
		}

		~string() {
			__RemoveReference();
		}

		string() {
			text = nullptr;
			length = -1;
			references = nullptr;
		}

		string(const string& other) {
			this->text = other.text;
			this->length = other.length;
			references = other.references;
			__AddReference();
		}

		string(const char* text) {
			if (text == nullptr) {
				text = nullptr;
				length = -1;
				references = nullptr;
			}
			else
			{
				this->length = strlen(text);
				this->text = new char[this->length + 1];
				ZeroMemory(this->text, this->length + 1);

				strcpy_s(this->text, this->length + 1, text);
				this->references = new int(1);
			}
		}

		string(const char* text, int count) {
			this->length = count;
			this->text = new char[this->length + 1];
			ZeroMemory(this->text, this->length + 1);

			for (int i = 0; i < count; i++)
				this->text[i] = text[i];
			this->references = new int(1);
		}

		bool operator == (const string& other) {
			if (this->IsNull() && other.IsNull())
				return true;

			if (this->IsNull() || other.IsNull())
				return false;

			if (other.text == this->text)
				return true; // same instance

			if (other.length != this->length)
				return false;

			for (int i = 0; i < this->length; i++)
				if (this->text[i] != other.text[i])
					return false;
			return true;
		}

		bool operator != (const string& other) {
			if (this->IsNull() && other.IsNull())
				return false;

			if (this->IsNull() || other.IsNull())
				return true;

			if (other.text == this->text)
				return false; // same instance

			if (other.length != this->length)
				return true;

			for (int i = 0; i < this->length; i++)
				if (this->text[i] != other.text[i])
					return true;
			return false;
		}

		string operator + (const string& other) {
			if (this->IsNull())
				return other;
			if (other.IsNull())
				return *this;

			string result;
			result.text = new char[this->length + other.length + 1];
			ZeroMemory(result.text, this->length + other.length + 1);
			strcpy_s(result.text, this->length + 1, this->text);
			strcpy_s(&result.text[this->length], other.length + 1, other.text);
			result.length = this->length + other.length;
			result.references = new int(1);
			return result;
		}

		string substr(int start, int count) {
			if (this->IsNull())
				throw new CA4G::CA4GException("Null reference to string");

			if (start + count > this->length)
				throw new CA4G::CA4GException("Argument error");

			return string(this->text + start, count);
		}

		string substr(int start) {
			if (this->IsNull())
				throw new CA4G::CA4GException("Null reference to string");

			if (start >= this->length)
				throw new CA4G::CA4GException("Argument error");

			return string(this->text + start, this->length - start);
		}

		int find_last_of(const string& sub) {
			if (sub.IsNull())
				return -1;
			if (this->length < sub.length)
				return -1;
			for (int i = this->length - sub.length; i >= 0; i--)
			{
				bool found = true;
				for (int j = 0; found && j < sub.length; j++)
					if (this->text[i + j] != sub.text[j])
						found = false;
				if (found)
					return i;
			}
			return -1;
		}

		string& operator = (const string& other) {
			__RemoveReference();
			this->references = other.references;
			this->length = other.length;
			this->text = other.text;
			__AddReference();
			return *this;
		}

		inline int len() { return this->length; }

		inline const char* c_str() { return this->text; }
	};

	template<typename T>
	class list
	{
		T* elements;
		int count;
		int capacity;
	public:
		list() {
			capacity = 32;
			count = 0;
			elements = new T[capacity];
			ZeroMemory(elements, sizeof(T) * capacity);
		}
		list(int capacity) {
			this->capacity = capacity;
			count = 0;
			elements = new T[capacity];
			ZeroMemory(elements, sizeof(T) * capacity);
		}
		list(const list<T>& other) {
			this->count = other.count;
			this->elements = new T[other.capacity];
			this->capacity = other.capacity;
			ZeroMemory(elements, sizeof(T) * capacity);
			for (int i = 0; i < this->capacity; i++)
				this->elements[i] = other.elements[i];
		}
	public:

		gObj<list<T>> clone() {
			gObj<list<T>> result = new list<T>();
			for (int i = 0; i < count; i++)
				result->add(elements[i]);
			return result;
		}

		void reset() {
			count = 0;
		}

		list(std::initializer_list<T> initialElements) {
			capacity = max(32, initialElements.size());
			count = initialElements.size();
			elements = new T[capacity];
			ZeroMemory(elements, sizeof(T) * capacity);

			for (int i = 0; i < initialElements.size(); i++)
				elements[i] = initialElements[i];
		}

		~list() {
			delete[] elements;
		}

		/// pushes an item at the begining
		void push(T item) {
			add(item);
			for (int i = count - 1; i > 0; i--)
				elements[i] = elements[i - 1];
			elements[0] = item;
		}

		int add(T item) {
			if (count == capacity)
			{
				capacity = (int)(capacity * 1.3);
				T* newelements = new T[capacity];
				ZeroMemory(newelements, sizeof(T) * capacity);

				for (int i = 0; i < count; i++)
					newelements[i] = elements[i];
				delete[] elements;
				elements = newelements;
			}
			elements[count] = item;
			return count++;
		}

		inline T& operator[](int index) const {
			return elements[index];
		}

		inline T& first() const {
			return elements[0];
		}

		inline T& last() const {
			return elements[count - 1];
		}

		inline int size() const {
			return count;
		}
	};


	template<typename T>
	class table
	{
		T* elements;
		int count;
		int capacity;
		int* references;
	public:
		table() {
			elements = nullptr;
			references = nullptr;
			count = capacity = 0;
		}
		table(int capacity) {
			this->capacity = capacity;
			count = 0;
			elements = new T[capacity];
			ZeroMemory(elements, sizeof(T) * capacity);
			references = new int(1);
		}
		table(const table<T>& other) {
			this->count = other.count;
			this->elements = other.elements;
			this->capacity = other.capacity;
			this->references = other.references;
			__AddReference();
		}
		void __RemoveReference() {
			if (references) // not null
			{
				if (--(*references) == 0)
					delete[] elements;
			}
		}
		void __AddReference() {
			if (this->references)
				++(*this->references);
		}
	public:

		void reset() {
			count = 0;
			ZeroMemory(elements, sizeof(T) * capacity);
		}

		table<T>& operator = (const table<T>& other) {
			__RemoveReference();
			this->references = other.references;
			this->length = other.length;
			this->text = other.text;
			__AddReference();
			return *this;
		}

		~table() {
			__RemoveReference();
		}

		/// pushes an item at the begining
		void push(T item) {
			add(item);
			for (int i = count - 1; i > 0; i--)
				elements[i] = elements[i - 1];
			elements[0] = item;
		}

		void add(T item) {
			if (count == capacity)
				throw CA4GException("Table overflow.");
			elements[count++] = item;
		}

		inline T& operator[](int index) const {
			return elements[index];
		}

		inline T& first() const {
			return elements[0];
		}

		inline T& last() const {
			return elements[count - 1];
		}

		inline int size() const {
			return count;
		}
	};
}


#endif