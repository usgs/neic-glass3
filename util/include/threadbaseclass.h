/*****************************************
 * This file is documented for Doxygen.
 * If you modify this file please update
 * the comments so that Doxygen will still
 * be able to work.
 ****************************************/
#ifndef THREADBASECLASS_H
#define THREADBASECLASS_H

#include <baseclass.h>
#include <thread>
#include <mutex>
#include <string>
#include <ctime>

namespace glass3 {
namespace util {
/**
 * \brief util threadbaseclass class
 *
 * This class supports creating, starting, stopping, and monitoring a single
 * work thread, allowing for specific thread name and sleep between work time.
 *
 * It is intended that the derived class implement the desired thread work by
 * overriding the pure virtual function work()
 *
 * This class inherits from util::baseclass
 */
class ThreadBaseClass : public util::BaseClass {
 public:
	/**
	 * \brief threadbaseclass constructor
	 *
	 * The constructor for the threadbaseclass class.
	 * Initializes members to default values.
	 */
	ThreadBaseClass();

	/**
	 * \brief  An advanced constructor that sets up the threadbaseclass with
	 * a provided thread name and sleep between work time
	 *
	 * The advanced constructor for the threadbaseclass class.
	 * Initializes members to provided values.
	 *
	 * \param threadname - A std::string containing the desired
	 * name of the work thread.
	 * \param sleeptimems - An integer value containing the amount of
	 * time to sleep between work() calls in the work thread
	 */
	ThreadBaseClass(std::string threadname, int sleeptimems);

	/**
	 * \brief threadbaseclass destructor
	 *
	 * The destructor for the threadbaseclass class.
	 */
	~ThreadBaseClass();

	/**
	 * \brief work thread start function
	 *
	 * Creates a thread object to run the workLoop() function, and starts it,
	 * setting m_bStarted to be true.
	 *
	 * \return returns true if successful, false if the thread creation failed
	 * or if a thread had already been started
	 */
	virtual bool start();

	/**
	 * \brief work thread stop function
	 *
	 * Stops, waits for, and deletes the thread that runs the workLoop() function,
	 * setting m_bStarted, m_bRunWorkThread, and m_bCheckWorkThread to false
	 *
	 * \return returns true if successful, false if the thread is not created and
	 * running
	 */
	virtual bool stop();

	/**
	 * \brief work thread check function
	 *
	 * Checks to see if the thread that runs the workLoop() function is still
	 * operational, by checking the value of m_bRunWorkThread (==true) every
	 * m_iCheckInterval seconds, setting m_bRunWorkThread to false after the
	 * check. It is expected that the derived class's overridden work() function
	 * periodically updates m_bRunWorkThread by calling setCheckWorkThread()
	 *
	 * \return returns true if thread is still running after m_iCheckInterval
	 * seconds, false otherwise
	 */
	virtual bool check();

	/**
	 * \brief Sets the time to sleep between work() calls
	 *
	 * Sets the amount of time to sleep between work() function calls in the
	 * workLoop() function, which is run by the thread
	 *
	 * \param sleeptimems - An integer value containing the sleep between work()
	 * calls in integer milliseconds.
	 */
	void setSleepTime(int sleeptimems);

	/**
	 * \brief Retrieves the time to sleep between work() calls
	 *
	 * Retrieves the amount of time to sleep between work() function calls in
	 * the workLoop() function, which is run by the thread
	 *
	 * \return Returns the amount of time to sleep between work() calls in
	 * integer milliseconds.
	 */
	int getSleepTime();

	/**
	 * \brief Checks to see if the work thread should still be running
	 *
	 * This function checks to see if the work thread should still running by
	 * returning the value of m_bRunWorkThread.
	 * \return Returns true if the thread should still running, false if it
	 * has been stopped
	 */
	virtual bool isRunning();

	/**
	 * \brief Function to set thread health
	 *
	 * This function signifies the thread health by setting m_bCheckWorkThread
	 * to the provided value.
	 *
	 * \param check = A boolean value indicating thread health, true indicates
	 * that the the thread is alive, false indicates that the thread is not
	 */
	void setCheckWorkThread(bool check = true);

	/**
	 * \brief Function to check thread health
	 *
	 * This function checks the thread health by getting the value of
	 * m_bCheckWorkThread.
	 *
	 * \return Returns true if the thread is alive, false if the thread has
	 * not responded yet
	 */
	bool getCheckWorkThread();

	/**
	 * \brief Function to set thread health check interval
	 *
	 * This function sets the time interval after which m_bCheckWorkThread being
	 * false (not responded) indicates that the thread has died in check()
	 *
	 * \param interval = An integer value indicating the thread health check
	 * interval in seconds
	 */
	void setCheckInterval(int interval);

	/**
	 * \brief Function to retrieve the thread health check interval
	 *
	 * This function retreives the time interval after which m_bCheckWorkThread
	 * being false (not responded) indicates that the thread has died in check()
	 *
	 * \return An integer value containing the thread health check interval in
	 * seconds
	 */
	int getCheckInterval();

	/**
	 *\brief Retrieves whether the work thread has been started
	 *
	 * This function retrieves the value of m_bStarted, which indicates whether
	 * the work thread has been created and started
	 * \returns true if the work thread has been started, false otherwise
	 */
	bool isStarted();

	/**
	 * \brief Function to set the name of the work thread
	 *
	 * This function sets the name of the work thread, this name is used to
	 * identify the work thread in logging
	 *
	 * \param name = A std::string containing the thread name to set
	 */
	void setThreadName(std::string name);

	/**
	 * \brief Function to retrieve the name of the work thread
	 *
	 * This function retrieves the name of the work thread, this name is used to
	 * identify the work thread
	 *
	 * \return A std::string containing the thread name
	 */
	const std::string& getThreadName();

	/**
	 * \brief Function to retrieve the last time the work thread health status
	 * was checked
	 *
	 * This function retrieves the last time the health status of this thread
	 * was checked by the check() function
	 *
	 * \return A std::time_t containing the last check time
	 */
	std::time_t getLastCheck();

	/**
	 * \brief threadbaseclass work function
	 *
	 * This function is a pure virtual work function. It is intended that the
	 * derived class implement the desired thread work by overriding this
	 * function
	 * \return This function returns true if the work was successful, false
	 * if it was not.
	 */
	virtual bool work() = 0;

 protected:
	/**
	 * \brief threadbaseclass work loop function
	 *
	 * This function is the thread work loop function. It runs in a loop while
	 * m_bRunWorkThread is true, calls work() every m_iSleepTimeMS milliseconds,
	 * to do a unit of work, sets thread health via setCheckWorkThread(), and
	 * sets m_bCheckWorkThread via setWorkCheck(). Setting m_bRunWorkThread
	 * to false via setRunning, or a false return from work() will cause
	 * the loop to exit, and the function to return (ending the thread)
	 */
	void workLoop();

	/**
	 * \brief Function to sset the last time the work thread health status
	 * was checked
	 *
	 * This function sets the last time the health status of this thread
	 * was checked.
	 *
	 * \param now - A std::time_t containing the last time the health status
	 * was checked
	 */
	void setLastCheck(std::time_t now);

	/**
	 *\brief Sets whether the work thread has been started
	 *
	 * This function sets the value of m_bStarted, which indicates whether
	 * the work thread has been created and started
	 * \param started - a boolean flag indicating whether the the thread has
	 * been started
	 */
	void setStarted(bool started);

	/**
	 * \brief Sets whether the work thread is running
	 *
	 * This function sets m_bRunWorkThread. Setting m_bRunWorkThread to true
	 * indicates that the thread should still run (via workloop()). Setting
	 * m_bRunWorkThread to false indicates that the thread should stop (and
	 * workloop() should return)
	 */
	void setRunning(bool running);

 private:
	/**
	 * \brief the std::string containing the name of the work thread,
	 * used for logging
	 */
	std::string m_sThreadName;

	/**
	 * \brief the integer interval in seconds after which the work thread
	 * will be considered dead. A negative check interval disables thread
	 * status checks
	 */
	int m_iCheckInterval;

	/**
	 * \brief the std::thread pointer to the work thread, allocated by start()
	 * and destroyed by stop()
	 */
	std::thread *m_WorkThread;

	/**
	 * \brief boolean flag indicating whether the work thread should run,
	 * controls the loop in workloop()
	 */
	bool m_bRunWorkThread;

	/**
	 * \brief boolean flag indicating whether the work thread has been started,
	 * set via setRunning() in start()
	 */
	bool m_bStarted;

	/**
	 * \brief boolean flag used to check thread status, set by
	 * setCheckWorkThread()
	 */
	bool m_bCheckWorkThread;

	/**
	 * \brief the std::mutex used to control access to m_bCheckWorkThread
	 */
	std::mutex m_CheckMutex;

	/**
	 * \brief the time_t holding the last time the thread status was checked,
	 * set by setLastCheck() in check
	 */
	time_t m_tLastCheck;

	/**
	 * \brief integer variable indicating how long to sleep in milliseconds
	 * between work() calls in workloop()
	 */
	int m_iSleepTimeMS;
};
}  // namespace util
}  // namespace glass3
#endif  // THREADBASECLASS_H
