#ifndef _SINGLETON_H_
#define _SINGLETON_H_

template <typename T>
class Singleton {
public:
	static T* getInstance() {
		if (m_pInstance == NULL) {
			m_pInstance = new T();
		}
		return m_pInstance;
	}
protected:
	static T* m_pInstance;
};

template <typename T> T* Singleton<T>::m_pInstance = NULL;

#endif
