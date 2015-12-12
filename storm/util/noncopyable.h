#ifndef _NONCOPYABLE_H_
#define _NONCOPYABLE_H_

class noncopyable {
protected:
	noncopyable() {}
	~noncopyable() {}
private:
	noncopyable(const noncopyable&);
	const noncopyable& operator= (const noncopyable&);
};

#endif
