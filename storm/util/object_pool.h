#ifndef _STORM_OBJECT_POOL_H_
#define _STORM_OBJECT_POOL_H_

#include <list>
#include <stdint.h>
#include <memory>

namespace Storm {
template <typename T>
class ObjectPool {
public:
	typedef std::shared_ptr<T> Tptr;
	ObjectPool(uint32_t iNodeNum = 128):m_iNodeNum(iNodeNum) {
		expandNode();
	}

	Tptr get() {
		if (m_freeNode.empty()) {
			expandNode();
		}
		Tptr pNode = m_freeNode.front();
		m_freeNode.pop_front();
		return pNode;
	}

	void put(Tptr pNode) {
		m_freeNode.push_back(pNode);
	}

private:
	void expandNode() {
		for (uint32_t i = 0; i < m_iNodeNum; ++i) {
			Tptr pTmp(new T);
			m_freeNode.push_back(pTmp);
		}
	}
private:
	uint32_t m_iNodeNum;
	std::list<Tptr> m_freeNode;
};
}

#endif
