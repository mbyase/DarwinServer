#include "motion.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <unistd.h>

#if defined(DARWIN) || defined(ROBOTIS)
#include <DARwIn.h>
#include <LinuxCM730.h>
#include <LinuxDARwIn.h>

using namespace Robot;

#endif // DARWIN || ROBOTIS

#ifdef DARWIN
const char* INI_FILE_PATH = "/darwin/Data/config.ini";
const char* U2D_DEV_NAME = "/dev/ttyUSB0";
const char* MOTION_FILE_PATH = "/darwin/Data/motion_4096.bin";
#endif // DARWIN

#ifdef ROBOTIS
const char* INI_FILE_PATH = "/robotis/Data/config.ini";
const char* U2D_DEV_NAME = "/dev/ttyUSB0";
const char* MOTION_FILE_PATH = "/robotis/Data/motion_4096.bin";
#endif // ROBOTIS

Motion& Motion::ins()
{
    static Motion unique_instance;
    return unique_instance;
}

Motion::Motion()
#if defined(DARWIN) || defined(ROBOTIS)
    : ini(new minIni(INI_FILE_PATH))
    , linux_cm730(U2D_DEV_NAME)
    , cm730(&linux_cm730)
#endif // DARWIN

{
    pthread_mutex_init(&mlock, NULL);
#if defined(DARWIN) || defined(ROBOTIS)
    Action::GetInstance()->LoadFile(const_cast<char*>(MOTION_FILE_PATH));

    assert(MotionManager::GetInstance()->Initialize(&cm730));
    Walking::GetInstance()->Initialize();
    MotionManager::GetInstance()->LoadINISettings(ini);
    Walking::GetInstance()->LoadINISettings(ini);

    MotionManager::GetInstance()->AddModule((MotionModule*)Action::GetInstance());
    MotionManager::GetInstance()->AddModule((MotionModule*)Head::GetInstance());
    MotionManager::GetInstance()->AddModule((MotionModule*)Walking::GetInstance());

    motion_timer = new LinuxMotionTimer(MotionManager::GetInstance());
    motion_timer->Start();
    MotionManager::GetInstance()->SetEnable(true);
    //Slowly stand up
    Action::GetInstance()->m_Joint.SetEnableBody(true, true);
    Action::GetInstance()->Start(15);
    while (Action::GetInstance()->IsRunning() == true)
        usleep(8000);
        //Head::GetInstance()->m_Joint.SetEnableHeadOnly(true, true);
        //Walking::GetInstance()->m_Joint.SetEnableBodyWithoutHead(true, true);
        //Head::GetInstance()->MoveByAngle(0, 60);
#else
    std::cout << "Call Motion constructor" << std::endl;
#endif
}

Motion::~Motion()
{
#if defined(DARWIN) || defined(ROBOTIS)
    Walking::GetInstance()->Stop();
    while (Walking::GetInstance()->IsRunning())
        usleep(8 * 1000);
    MotionManager::GetInstance()->RemoveModule((MotionModule*)Walking::GetInstance());
    Action::GetInstance()->Start(15);
    while (Action::GetInstance()->IsRunning())
        usleep(8 * 1000);
    delete motion_timer;
    delete ini;
#else
    std::cout << "Call Motion destructor" << std::endl;
#endif // DARWIN
    pthread_mutex_destroy(&mlock);
}

bool Motion::walk_start()
{
    pthread_mutex_lock(&mlock);
    bool ret;
#if defined(DARWIN) || defined(ROBOTIS)
    //Slowly stand up
    Action::GetInstance()->m_Joint.SetEnableBody(true, true);
    Action::GetInstance()->Start(9);
    while (Action::GetInstance()->IsRunning() == true)
        usleep(8000);
    Walking::GetInstance()->m_Joint.SetEnableBodyWithoutHead(true, true);
    Robot::Walking::GetInstance()->Start();
    ret = true;
#else
    std::cout << "call Motion::walk_start()" << std::endl;
    ret = false;
#endif // DARWIN
    pthread_mutex_unlock(&mlock);
    return ret;
}

bool Motion::walk_stop()
{
    bool ret;
    pthread_mutex_lock(&mlock);
#if defined(DARWIN) || defined(ROBOTIS)
    Walking::GetInstance()->Stop();
    while (Walking::GetInstance()->IsRunning())
        usleep(8 * 1000);
    ret = true;
#else
    std::cout << "Call Motion::walk_stop()" << std::endl;
    ret = false;
#endif // DARWIN
    pthread_mutex_unlock(&mlock);
    return ret;
}

bool Motion::walk(int x_move, int a_move, int msec)
{
    bool ret;
    pthread_mutex_lock(&mlock);
#if defined(DARWIN) || defined(ROBOTIS)
    if (msec == 0) {
        Robot::Walking::GetInstance()->X_MOVE_AMPLITUDE = x_move; //Straight speed
        Robot::Walking::GetInstance()->A_MOVE_AMPLITUDE = a_move; //Turn speed
    } else {
        int pre_amp = Robot::Walking::GetInstance()->X_MOVE_AMPLITUDE;
        int iterval = msec / abs(pre_amp - x_move);
        for (int i = pre_amp; i != x_move;) {
            Robot::Walking::GetInstance()->X_MOVE_AMPLITUDE = i;
            usleep(iterval * 1000);
            if (pre_amp > x_move) {
                --i;
            } else {
                ++i;
            }
        }
    }
    ret = true;
#else
    std::cout << "Call Motion::walk( " << x_move << ", " << a_move << ", " << msec << " )" << std::endl;
    ret = false;
#endif // DARWIN
    pthread_mutex_unlock(&mlock);
    return ret;
}

bool Motion::fall_up()
{
    pthread_mutex_lock(&mlock);
#if defined(DARWIN) || defined(ROBOTIS)
    if (MotionStatus::FALLEN != STANDUP) {
        Walking::GetInstance()->Stop();
        while (Walking::GetInstance()->IsRunning() == 1)
            usleep(8000);

        Action::GetInstance()->m_Joint.SetEnableBody(true, true);

        if (MotionStatus::FALLEN == FORWARD)
            Action::GetInstance()->Start(10); //Forwrad getup
        else if (MotionStatus::FALLEN == BACKWARD)
            Action::GetInstance()->Start(11); //Backward getup

        while (Action::GetInstance()->IsRunning() == 1)
            usleep(8000);

        Head::GetInstance()->m_Joint.SetEnableHeadOnly(true, true);
        Walking::GetInstance()->m_Joint.SetEnableBodyWithoutHead(true, true);
        pthread_mutex_unlock(&mlock);
        return true;
    }
#else
    std::cout << "Call Motion::walk_stop()" << std::endl;
#endif // DARWIN
    pthread_mutex_unlock(&mlock);
    return false;
}

bool Motion::head_move(int x, int y, bool home)
{
    bool ret;
    pthread_mutex_lock(&mlock);
#if defined(DARWIN) || defined(ROBOTIS)
    Head::GetInstance()->m_Joint.SetEnableHeadOnly(true, true);
    if (home)
        Head::GetInstance()->MoveToHome();
    Head::GetInstance()->MoveByAngle(x, y);
    ret = true;
#else
    std::cout << "Call Motion::head_move( " << x << ", " << y << ", " << home << " )" << std::endl;
    ret = false;
#endif // DARWIN
    pthread_mutex_unlock(&mlock);
    return ret;
}

bool Motion::action(int index, const std::string& audio)
{
    bool ret;
    pthread_mutex_lock(&mlock);
#if defined(DARWIN) || defined(ROBOTIS)
    Walking::GetInstance()->Stop();
    while (Walking::GetInstance()->IsRunning() == 1)
        usleep(8000);
    Action::GetInstance()->m_Joint.SetEnableBody(true, true);

    Action::GetInstance()->Start(index);
    if (!audio.empty()) {
        Robot::LinuxActionScript::PlayMP3(audio.c_str());
    }

    while (Action::GetInstance()->IsRunning() == 1)
        usleep(8000);
    ret = true;
#else
    std::cout << "Call Motion::action( " << index << ", " << audio << " )" << std::endl;
    ret = false;
#endif // DARWIN
    pthread_mutex_unlock(&mlock);
    return ret;
}
