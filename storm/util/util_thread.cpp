#include "util_thread.h"

#include <sys/time.h>
#include <time.h>

#include "util_time.h"


namespace Storm {
Notifier::Notifier() {
	pthread_cond_init(&m_cond, NULL);
}

Notifier::~Notifier() {
	pthread_cond_destroy(&m_cond);
}

void Notifier::signal() {
	pthread_cond_signal(&m_cond);
}

void Notifier::broadcast() {
	pthread_cond_broadcast(&m_cond);
}

void Notifier::wait() {
	pthread_cond_wait(&m_cond, &m_lock.m_mutex);
}

void Notifier::timedwait(uint64_t ms) {
	ms += UtilTime::getNowMS();
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000 * 1000;
	pthread_cond_timedwait(&m_cond, &m_lock.m_mutex, &ts);
}

void* Thread::threadEntry(void* argc) {
	Thread* pThread = static_cast<Thread*>(argc);
	pthread_detach(pthread_self());
	pThread->m_entry();

	return (void*)NULL;
}

void Thread::start(boost::function<void (void) > entry) {
	if (m_bRunning) {
		return;
	}
	m_entry = entry;
	m_bRunning = true;
	pthread_create(&m_thread, NULL, threadEntry, this);
}

void Thread::join() {
	if (!m_bRunning) {
		return;
	}
	pthread_join(m_thread, NULL);
}
}
