#ifndef CA4G_COLLECTIONS_H
#define CA4G_COLLECTIONS_H

namespace CA4G {

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
}


#endif