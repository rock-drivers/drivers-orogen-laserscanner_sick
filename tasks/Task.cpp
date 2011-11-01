/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace laserscanner_sick;

Task::Task(std::string const& name, TaskCore::TaskState initial_state)
    : TaskBase(name, initial_state)
    , sick(0)
{
    status = 1;
    num_measurements = 0; 
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine, TaskCore::TaskState initial_state)
    : TaskBase(name, engine, initial_state)
    , sick(0)
{
    status = 1;
    num_measurements = 0;
}

Task::~Task()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;
    sick = new SickToolbox::SickLMS1xx(_hostname.get(), _port.get()); 

    try{
	sick->Initialize();
    }
    catch(...){
	cerr << cerr << "Initialize failed! Are you using the correct IP address?" << endl;
	return false;
    }

    try{
	sick->SetSickScanDataFormat(SickToolbox::SickLMS1xx::SICK_LMS_1XX_SCAN_FORMAT_DIST_DOUBLE_PULSE_REFLECT_16BIT);
    }
    catch(SickToolbox::SickConfigException sick_exception) {
	std::cout << sick_exception.what() << std::endl;
	return false;
    }
    return true;
}

// bool Task::startHook()
// {
//     if (! TaskBase::startHook())
//         return false;
//     return true;
// }

void Task::updateHook()
{
    TaskBase::updateHook();
    base::samples::LaserScan scan;

    try{
	sick->GetSickMeasurements(range_1_vals,range_2_vals,range_1_vals,range_2_vals,num_measurements,&status);
	for(int i=0; i<num_measurements; i++){
	    scan.ranges.push_back(range_1_vals[i]);
	    //std::cout << range_1_vals[i] << std::endl;
	}
	for(int i=0; i<num_measurements; i++){
	    scan.remission.push_back(range_2_vals[i]);
	}
    }
    catch(SickToolbox::SickIOException sick_exception) {
	std::cout << sick_exception.what() << std::endl;
	exception();
	return;
    }
    catch(SickToolbox::SickTimeoutException sick_exception) {
	std::cout << sick_exception.what() << std::endl;
	exception();
	return;
    }
    catch(...) {
	cerr << "An Error Occurred!" << endl;
	exception();
	return;
    }
    scan.time = base::Time::now();
    scan.start_angle = -M_PI;
    scan.angular_resolution = 0.001;
    scan.speed = 0.001;
    scan.minRange = 50; 
    scan.maxRange = 50000;

    _scan.write(scan);
}

// void Task::errorHook()
// {
//     TaskBase::errorHook();
// }
// void Task::stopHook()
// {
//     TaskBase::stopHook();
// }

void Task::cleanupHook()
{
    TaskBase::cleanupHook();
    if(sick)
        delete sick;
    sick = 0;
}

